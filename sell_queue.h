#ifndef SELL_QUEUE_H
#define SELL_QUEUE_H

#define SELL_QUEUE_MAX 32

/* 업그레이드 이전 상태를 저장하는 구조체
 * 판매(환불) 시 해당 타워를 이전 스탯으로 복원하는 데 사용 */
typedef struct {
    int   towerIdx;   // 업그레이드된 타워 인덱스
    int   prevLevel;  // 업그레이드 이전 레벨
    float prevDamage; // 업그레이드 이전 데미지
    float prevRange;  // 업그레이드 이전 사거리
    float prevRate;   // 업그레이드 이전 공격속도
} UpgradeInfo;

/* 업그레이드 이력 관리용 원형 큐 구조체
 * 보스 경로 순서(=업그레이드 순서)대로 판매 가능하도록 FIFO로 관리 */
typedef struct {
    UpgradeInfo data[SELL_QUEUE_MAX];
    int front, rear, size;
} SellQueue;

void        SellQueue_Init(SellQueue *q);          // 큐 초기화
void        SellQueue_Clear(SellQueue *q);          // 큐 전체 비우기
int         SellQueue_IsEmpty(const SellQueue *q);  // 큐가 비어있는지 확인
void        SellQueue_Enqueue(SellQueue *q, UpgradeInfo info); // 업그레이드 이력 삽입
UpgradeInfo SellQueue_Dequeue(SellQueue *q);                   // 업그레이드 이력 추출
int         SellQueue_ContainsTower(const SellQueue *q, int towerIdx); // front 타워 일치 여부 확인

#endif
