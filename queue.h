#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_MAX 128

/* 웨이브 스폰에 사용되는 적 정보 구조체 */
typedef struct {
    float hp;
    float speed;
    int isBoss; // 0: 일반 몬스터, 1: 보스
} EnemyInfo;

/* 원형 큐(Circular Queue) 구조체 - 웨이브 스폰 순서 관리용 */
typedef struct {
    EnemyInfo data[QUEUE_MAX];
    int front, rear, size;
} Queue;

void      Queue_Init(Queue* q);          // 큐 초기화
void      Queue_Clear(Queue* q);         // 큐 전체 비우기
int       Queue_IsEmpty(const Queue* q); // 큐가 비어있는지 확인
void      Queue_Enqueue(Queue* q, EnemyInfo info); // 적 정보 삽입 (rear)
EnemyInfo Queue_Dequeue(Queue* q);                 // 적 정보 추출 (front)

#endif
