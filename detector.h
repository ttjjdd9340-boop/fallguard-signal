#ifndef DETECTOR_H
#define DETECTOR_H

#include "sample.h"
#include "ring_buffer.h"
#include "low_pass.h"

#define SAMPLE_RATE_HZ       100
#define ALPHA                0.3f
#define CANDIDATE_ACC_G      1.8f    /* XGBoost 대신 쓰는 임시 후보 기준 */
#define IMMEDIATE_ACC_G      2.5f    /* 이 이상이면 즉시 알림 (Logic A) */
#define IMMEDIATE_GYR_DPS    200.0f
#define CAPTURE_SAMPLES      30      /* 후보 발견 뒤 충격이 지나가는 0.3초를 더 지켜본다 */
#define BUTTON_WAIT_SAMPLES  (15 * SAMPLE_RATE_HZ)   /* 15초 */
#define COOLDOWN_SAMPLES     (3 * SAMPLE_RATE_HZ)    /* 3초 */

typedef enum { DET_IDLE, DET_CAPTURING, DET_WAITING, DET_COOLDOWN } DetState;

typedef enum {
    EV_NONE,
    EV_CONFIRMED_A,    /* Logic A: 즉시 알림, 낙상 확정 */
    EV_WAIT_STARTED,   /* Logic B: 15초 취소 대기 시작 */
    EV_CANCELLED,      /* 대기 중 버튼 입력: 오탐 */
    EV_CONFIRMED_B     /* 대기 시간 동안 응답 없음: 낙상 확정 */
} Event;

typedef struct {
    RingBuffer buf;
    LowPass    filt[6];
    DetState   state;
    int        timer;           /* 관찰, 대기, 쿨다운에서 남은 샘플 수 */
    float      last_peak_acc;   /* 마지막 후보의 가속도 최댓값 */
    float      last_peak_gyr;   /* 마지막 후보의 회전 최댓값 */
} Detector;

void  det_init(Detector *d);
Event det_update(Detector *d, Sample raw, int button_pressed);

#endif
