#include "queue.h"
#include <stdio.h>

/* 큐 초기화 - front, rear, size를 0으로 설정 */
void Queue_Init(Queue *q) {
    q->front = q->rear = q->size = 0;
}

/* 큐 전체 비우기 - 포인터와 크기를 초기 상태로 리셋 */
void Queue_Clear(Queue *q) {
    q->front = q->rear = q->size = 0;
}

/* 큐가 비어있는지 여부 반환 (비어있으면 1, 아니면 0) */
int Queue_IsEmpty(const Queue *q) {
    return q->size == 0;
}

/* 적 정보를 큐의 rear에 삽입
 * 원형 큐 방식으로 rear를 순환시켜 배열 끝 이후 앞으로 돌아옴
 * 최대 용량 초과 시 삽입 무시 */
void Queue_Enqueue(Queue *q, EnemyInfo info) {
    if (q->size >= QUEUE_MAX) return;
    q->data[q->rear] = info;
    q->rear = (q->rear + 1) % QUEUE_MAX;
    q->size++;
}

/* 큐의 front에서 적 정보를 꺼내 반환 (FIFO)
 * 비어있을 경우 0으로 초기화된 빈 구조체 반환 */
EnemyInfo Queue_Dequeue(Queue *q) {
    EnemyInfo info = {0};
    if (Queue_IsEmpty(q)) return info;
    info = q->data[q->front];
    q->front = (q->front + 1) % QUEUE_MAX;
    q->size--;
    return info;
}
