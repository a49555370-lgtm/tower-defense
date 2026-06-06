#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#define PQ_MAX 64

/* 우선순위 큐의 노드 구조체
 * priority가 낮을수록 우선순위 높음 (거리 기준 min-heap) */
typedef struct {
    int   enemyIdx; // 적 배열 인덱스
    float priority; // 우선순위 값 (타워와 적 사이의 거리)
} PQNode;

/* Min-heap 기반 우선순위 큐 구조체 - 타워 타겟팅용
 * 사거리 내 적 중 가장 가까운 적을 O(log n)으로 추출 */
typedef struct {
    PQNode heap[PQ_MAX];
    int    size;
} PriorityQueue;

void PQ_Init(PriorityQueue *pq);                              // 큐 초기화
void PQ_Clear(PriorityQueue *pq);                             // 큐 전체 비우기
int  PQ_IsEmpty(const PriorityQueue *pq);                     // 큐가 비어있는지 확인
void PQ_Insert(PriorityQueue *pq, int enemyIdx, float priority); // 적 삽입 후 sift-up
int  PQ_ExtractMin(PriorityQueue *pq);                        // 최소 우선순위 적 인덱스 추출

#endif
