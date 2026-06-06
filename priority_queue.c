#include "priority_queue.h"

/* 우선순위 큐 초기화 */
void PQ_Init(PriorityQueue *pq)  { pq->size = 0; }

/* 우선순위 큐 전체 비우기 - 매 프레임 타겟 재탐색 전 호출 */
void PQ_Clear(PriorityQueue *pq) { pq->size = 0; }

/* 큐가 비어있는지 여부 반환 */
int  PQ_IsEmpty(const PriorityQueue *pq) { return pq->size == 0; }

/* 두 노드의 값을 교환하는 내부 헬퍼 함수 */
static void Swap(PQNode *a, PQNode *b) { PQNode t = *a; *a = *b; *b = t; }

/* 적을 우선순위 큐에 삽입
 * 삽입 후 sift-up으로 min-heap 성질 유지
 * priority가 낮을수록(=거리가 가까울수록) 루트에 위치 */
void PQ_Insert(PriorityQueue *pq, int enemyIdx, float priority) {
    if (pq->size >= PQ_MAX) return;
    int i = pq->size++;
    pq->heap[i].enemyIdx = enemyIdx;
    pq->heap[i].priority = priority;
    /* Sift up: 부모보다 우선순위가 높으면(값이 작으면) 교환 반복 */
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->heap[parent].priority > pq->heap[i].priority) {
            Swap(&pq->heap[parent], &pq->heap[i]);
            i = parent;
        } else break;
    }
}

/* 가장 가까운 적(최소 priority)의 인덱스를 추출
 * 루트 제거 후 마지막 노드를 루트로 올리고 sift-down으로 heap 복원
 * 비어있을 경우 -1 반환 */
int PQ_ExtractMin(PriorityQueue *pq) {
    if (PQ_IsEmpty(pq)) return -1;
    int result = pq->heap[0].enemyIdx;
    pq->heap[0] = pq->heap[--pq->size];
    /* Sift down: 자식 중 더 작은 값과 교환 반복 */
    int i = 0;
    while (1) {
        int left = 2*i+1, right = 2*i+2, smallest = i;
        if (left  < pq->size && pq->heap[left].priority  < pq->heap[smallest].priority) smallest = left;
        if (right < pq->size && pq->heap[right].priority < pq->heap[smallest].priority) smallest = right;
        if (smallest == i) break;
        Swap(&pq->heap[i], &pq->heap[smallest]);
        i = smallest;
    }
    return result;
}
