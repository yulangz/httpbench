//
// Created by yulan on 2021/5/8.
//

#include "utils.h"

int CQ_init(struct CircularQueue *q, size_t size) {
    q->front = q->back = 0;
    q->size = size + 1;
    q->queue = (QUEUE_ELEMENT_TYPE *) malloc((size + 1) * sizeof(QUEUE_ELEMENT_TYPE));
    if (q->queue == NULL)
        return -1;
    return 0;
}

void CQ_destroy(struct CircularQueue *q) {
    Free(q->queue);
}

int CQ_in_queue(struct CircularQueue *q, QUEUE_ELEMENT_TYPE v) {
    if ((q->back + 1) % q->size == q->front)
        return -1;
    q->queue[q->back] = v;
    q->back = (q->back + 1) % q->size;
    return 0;
}

int CQ_pop_queue(struct CircularQueue *q, QUEUE_ELEMENT_TYPE *out) {
    if (q->front == q->back)
        return -1;
    *out = q->queue[q->front];
    q->front = (q->front + 1) % q->size;
    return 0;
}

int CQ_empty(struct CircularQueue *q) {
    return q->front == q->back;
}

size_t CQ_get_size(struct CircularQueue *q) {
    return (q->back + q->size - q->front) % q->size;
}