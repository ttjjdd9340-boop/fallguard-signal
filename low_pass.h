#ifndef LOW_PASS_H
#define LOW_PASS_H

/* 저역통과 필터 (지수 이동평균): 새 값 alpha + 이전 결과 (1-alpha) */
typedef struct {
    float alpha;
    float prev;
    int   ready;   /* 첫 값을 받았는지 */
} LowPass;

void  lp_init(LowPass *f, float alpha);
void  lp_reset(LowPass *f);
float lp_update(LowPass *f, float x);

#endif
