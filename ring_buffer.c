#include "ring_buffer.h"

void rb_init(RingBuffer *rb) {
    rb->head = 0;
    rb->count = 0;
}

void rb_clear(RingBuffer *rb) {
    rb_init(rb);
}

/* 새 값을 head 칸에 쓰고, head를 한 칸 앞으로 보낸다.
   끝칸을 지나면 % 때문에 0번으로 돌아간다 (그래서 "원형"). */
void rb_push(RingBuffer *rb, Sample s) {
    rb->data[rb->head] = s;
    rb->head = (rb->head + 1) % RB_CAPACITY;
    if (rb->count < RB_CAPACITY) {
        rb->count++;
    }
}

int rb_full(const RingBuffer *rb) {
    return rb->count == RB_CAPACITY;
}

/* 가장 오래된 값부터 i번째 값의 주소를 돌려준다 */
const Sample *rb_at(const RingBuffer *rb, int i) {
    int oldest = (rb->head - rb->count + RB_CAPACITY) % RB_CAPACITY;
    return &rb->data[(oldest + i) % RB_CAPACITY];
}
