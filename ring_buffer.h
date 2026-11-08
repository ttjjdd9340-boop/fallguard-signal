#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "sample.h"

#define RB_CAPACITY 100   /* 100Hz x 1초 = 최근 100개만 보관 */

typedef struct {
    Sample data[RB_CAPACITY];
    int head;    /* 다음에 쓸 칸 번호 */
    int count;   /* 지금 채워진 칸 수 */
} RingBuffer;

void rb_init(RingBuffer *rb);
void rb_clear(RingBuffer *rb);
void rb_push(RingBuffer *rb, Sample s);
int  rb_full(const RingBuffer *rb);
const Sample *rb_at(const RingBuffer *rb, int i);   /* i=0 이 가장 오래된 값 */

#endif
