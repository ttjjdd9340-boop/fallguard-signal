#include "low_pass.h"

void lp_init(LowPass *f, float alpha) {
    f->alpha = alpha;
    f->prev = 0.0f;
    f->ready = 0;
}

void lp_reset(LowPass *f) {
    f->prev = 0.0f;
    f->ready = 0;
}

float lp_update(LowPass *f, float x) {
    if (!f->ready) {          /* 첫 값은 그대로 기준으로 삼는다 */
        f->prev = x;
        f->ready = 1;
    }
    f->prev = f->alpha * x + (1.0f - f->alpha) * f->prev;
    return f->prev;
}
