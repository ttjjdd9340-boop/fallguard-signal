#ifndef SAMPLE_H
#define SAMPLE_H

/* 센서 한 번 읽은 값 (6축) */
typedef struct {
    float ax, ay, az;   /* 가속도 (g) */
    float gx, gy, gz;   /* 자이로 (deg/s) */
} Sample;

#endif
