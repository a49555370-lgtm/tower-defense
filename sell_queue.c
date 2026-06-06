#include "sell_queue.h"

/* 판매 큐 초기화 - front, rear, size를 0으로 설정 */
void SellQueue_Init(SellQueue *q) {
    q->front = q->rear = q->size = 0;
}

/* 판매 큐 전체 비우기 - 게임 재시작 시 호출 */
void SellQueue_Clear(SellQueue *q) {
    q->front = q->rear = q->size = 0;
}

/* 큐가 비어있는지 여부 반환 (비어있으면 1, 아니면 0) */
int SellQueue_IsEmpty(const SellQueue *q) {
    return q->size == 0;
}

/* 업그레이드 이력을 큐의 rear에 삽입
 * 타워 업그레이드 시 이전 스탯을 저장해 추후 환불에 활용
 * 최대 용량 초과 시 삽입 무시 */
void SellQueue_Enqueue(SellQueue *q, UpgradeInfo info) {
    if (q->size >= SELL_QUEUE_MAX) return;
    q->data[q->rear] = info;
    q->rear = (q->rear + 1) % SELL_QUEUE_MAX;
    q->size++;
}

/* 큐의 front에서 업그레이드 이력을 꺼내 반환 (FIFO)
 * 가장 먼저 업그레이드된 타워(보스가 먼저 지나친 타워)부터 환불 처리
 * 비어있을 경우 0으로 초기화된 빈 구조체 반환 */
UpgradeInfo SellQueue_Dequeue(SellQueue *q) {
    UpgradeInfo info = {0};
    if (SellQueue_IsEmpty(q)) return info;
    info = q->data[q->front];
    q->front = (q->front + 1) % SELL_QUEUE_MAX;
    q->size--;
    return info;
}

/* 큐의 front에 있는 항목이 특정 타워인지 확인
 * 판매 버튼 활성화 여부 판단에 사용 - front 타워만 판매 가능 */
int SellQueue_ContainsTower(const SellQueue *q, int towerIdx) {
    if (SellQueue_IsEmpty(q)) return 0;
    return q->data[q->front].towerIdx == towerIdx;
}
