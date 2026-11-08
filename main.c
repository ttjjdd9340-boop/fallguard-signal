#include <math.h>
#include <stdio.h>
#include <time.h>
#include "detector.h"

#define TOTAL_SECONDS 100
#define TOTAL_SAMPLES (TOTAL_SECONDS * SAMPLE_RATE_HZ)
#define IMPACT_SECONDS 0.3f
#define PI 3.14159265f

/* 합성 센서 데이터에 넣을 충격 상황 */
typedef struct {
    int   start_sec;         /* 충격이 시작되는 시각 */
    float peak_g;            /* 충격 때 가속도 최댓값 */
    float peak_dps;          /* 충격 때 회전 최댓값 */
    int   press_after_sec;   /* 대기 시작 후 몇 초 뒤 버튼을 누르나 (-1: 안 누름) */
} Scenario;

static const Scenario scenarios[] = {
    { 10, 2.2f, 100.0f,  5 },   /* 경미한 충격, 버튼을 눌러 취소 */
    { 40, 4.0f, 300.0f, -1 },   /* 큰 충격 */
    { 70, 2.2f, 100.0f, -1 },   /* 경미한 충격, 버튼 안 누름 */
};
#define SCENARIO_COUNT ((int)(sizeof(scenarios) / sizeof(scenarios[0])))

/* 매번 같은 값이 나오는 간단한 난수 (실험 결과가 재현되도록) */
static unsigned int rng_state = 12345u;
static float noise(float amp) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return ((float)((rng_state >> 8) & 0xFFFF) / 32768.0f - 1.0f) * amp;
}

/* n번째 샘플: 걷는 움직임 + 노이즈 + (있으면) 충격 */
static Sample make_sample(int n) {
    float t = (float)n / SAMPLE_RATE_HZ;
    Sample s;
    s.ax = noise(0.10f);
    s.ay = noise(0.10f);
    s.az = 1.0f + 0.15f * sinf(2.0f * PI * 2.0f * t) + noise(0.05f);
    s.gx = noise(10.0f);
    s.gy = noise(10.0f);
    s.gz = noise(10.0f);

    for (int i = 0; i < SCENARIO_COUNT; i++) {
        float dt = t - (float)scenarios[i].start_sec;
        if (dt >= 0.0f && dt < IMPACT_SECONDS) {
            float shape = sinf(PI * dt / IMPACT_SECONDS);   /* 0 -> 1 -> 0 */
            s.az += (scenarios[i].peak_g - 1.0f) * shape;
            s.gx += scenarios[i].peak_dps * shape;
        }
    }
    return s;
}

int main(void) {
    Detector det;
    det_init(&det);

    int confirmed = 0;
    int cancelled = 0;
    int press_at = -1;               /* 버튼을 누를 샘플 번호 */
    double total_ns = 0.0;

    for (int n = 0; n < TOTAL_SAMPLES; n++) {
        Sample s = make_sample(n);
        int button = (press_at >= 0 && n >= press_at);

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        Event ev = det_update(&det, s, button);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        total_ns += (double)(t1.tv_sec - t0.tv_sec) * 1e9 + (double)(t1.tv_nsec - t0.tv_nsec);

        double sec = (double)(n + 1) / SAMPLE_RATE_HZ;
        switch (ev) {
        case EV_WAIT_STARTED:
            printf("[%6.2fs] 낙상 후보 (가속도 %.2fg, 회전 %.0f°/s) -> Logic B: 15초 취소 대기\n",
                   sec, det.last_peak_acc, det.last_peak_gyr);
            press_at = -1;
            {
                /* 지금 상황에 해당하는 시나리오 = 시작 시각이 지금 이전인 것 중 가장 늦은 것 */
                int cur = -1;
                for (int i = 0; i < SCENARIO_COUNT; i++) {
                    if (scenarios[i].start_sec * SAMPLE_RATE_HZ <= n) {
                        cur = i;
                    }
                }
                if (cur >= 0 && scenarios[cur].press_after_sec >= 0) {
                    press_at = n + scenarios[cur].press_after_sec * SAMPLE_RATE_HZ;
                }
            }
            break;
        case EV_CANCELLED:
            printf("[%6.2fs] 버튼 입력 -> 오탐으로 기록\n", sec);
            cancelled++;
            press_at = -1;
            break;
        case EV_CONFIRMED_B:
            printf("[%6.2fs] 15초 동안 응답 없음 -> 낙상 확정\n", sec);
            confirmed++;
            press_at = -1;
            break;
        case EV_CONFIRMED_A:
            printf("[%6.2fs] 낙상 후보 (가속도 %.2fg, 회전 %.0f°/s) -> Logic A: 즉시 알림, 낙상 확정\n",
                   sec, det.last_peak_acc, det.last_peak_gyr);
            confirmed++;
            break;
        case EV_NONE:
            break;
        }
    }

    printf("\n--- 요약 ---\n");
    printf("처리한 샘플: %d개 (%d초 분량)\n", TOTAL_SAMPLES, TOTAL_SECONDS);
    printf("낙상 확정: %d회, 오탐 취소: %d회\n", confirmed, cancelled);
    printf("샘플 1개당 평균 처리 시간: %.2f us\n", total_ns / TOTAL_SAMPLES / 1000.0);
    return 0;
}
