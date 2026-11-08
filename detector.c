#include <math.h>
#include "detector.h"

static float mag3(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

void det_init(Detector *d) {
    rb_init(&d->buf);
    for (int i = 0; i < 6; i++) {
        lp_init(&d->filt[i], ALPHA);
    }
    d->state = DET_IDLE;
    d->timer = 0;
    d->last_peak_acc = 0.0f;
    d->last_peak_gyr = 0.0f;
}

/* 이벤트 처리 후: 버퍼와 필터 기억을 지운다 (같은 충격을 또 판정하지 않게) */
static void reset_signal(Detector *d) {
    rb_clear(&d->buf);
    for (int i = 0; i < 6; i++) {
        lp_reset(&d->filt[i]);
    }
}

static void start_cooldown(Detector *d) {
    d->state = DET_COOLDOWN;
    d->timer = COOLDOWN_SAMPLES;
    reset_signal(d);
}

/* 새 센서 값을 필터에 통과시켜 버퍼에 넣는다 */
static void push_filtered(Detector *d, Sample raw) {
    Sample f;
    f.ax = lp_update(&d->filt[0], raw.ax);
    f.ay = lp_update(&d->filt[1], raw.ay);
    f.az = lp_update(&d->filt[2], raw.az);
    f.gx = lp_update(&d->filt[3], raw.gx);
    f.gy = lp_update(&d->filt[4], raw.gy);
    f.gz = lp_update(&d->filt[5], raw.gz);
    rb_push(&d->buf, f);
}

/* 버퍼 안의 최근 100개 중 가속도와 회전의 최댓값 */
static void window_peaks(const Detector *d, float *acc_max, float *gyr_max) {
    float a = 0.0f;
    float g = 0.0f;
    for (int i = 0; i < d->buf.count; i++) {
        const Sample *s = rb_at(&d->buf, i);
        float am = mag3(s->ax, s->ay, s->az);
        float gm = mag3(s->gx, s->gy, s->gz);
        if (am > a) a = am;
        if (gm > g) g = gm;
    }
    *acc_max = a;
    *gyr_max = g;
}

/* 샘플 한 개가 들어올 때마다 한 번 호출한다.
   기다리는 동안에도 멈추지 않고 계속 호출되는 구조 (논블로킹) */
Event det_update(Detector *d, Sample raw, int button_pressed) {
    /* 1) 쿨다운: 정해진 시간 동안은 아무것도 안 한다 */
    if (d->state == DET_COOLDOWN) {
        if (--d->timer <= 0) {
            d->state = DET_IDLE;
        }
        return EV_NONE;
    }

    /* 2) 취소 대기: 버튼이 눌리면 오탐, 시간이 끝나면 확정 */
    if (d->state == DET_WAITING) {
        if (button_pressed) {
            start_cooldown(d);
            return EV_CANCELLED;
        }
        if (--d->timer <= 0) {
            start_cooldown(d);
            return EV_CONFIRMED_B;
        }
        return EV_NONE;
    }

    /* 3) 관찰: 후보를 발견한 뒤 충격이 지나가는 동안 최댓값을 더 지켜본다 */
    if (d->state == DET_CAPTURING) {
        push_filtered(d, raw);
        if (--d->timer > 0) {
            return EV_NONE;
        }
        float acc_max, gyr_max;
        window_peaks(d, &acc_max, &gyr_max);
        d->last_peak_acc = acc_max;
        d->last_peak_gyr = gyr_max;
        if (acc_max >= IMMEDIATE_ACC_G || gyr_max >= IMMEDIATE_GYR_DPS) {
            start_cooldown(d);
            return EV_CONFIRMED_A;    /* Logic A */
        }
        d->state = DET_WAITING;       /* Logic B */
        d->timer = BUTTON_WAIT_SAMPLES;
        return EV_WAIT_STARTED;
    }

    /* 4) 평소(IDLE): 필터 -> 버퍼 -> 최댓값 확인 */
    push_filtered(d, raw);
    if (!rb_full(&d->buf)) {
        return EV_NONE;       /* 아직 1초 분량이 안 모였다 */
    }

    float acc_max, gyr_max;
    window_peaks(d, &acc_max, &gyr_max);
    if (acc_max >= CANDIDATE_ACC_G) {
        d->state = DET_CAPTURING;     /* 후보 발견: 바로 판정하지 않고 더 지켜본다 */
        d->timer = CAPTURE_SAMPLES;
    }
    return EV_NONE;
}
