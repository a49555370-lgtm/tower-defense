#ifndef TREE_H
#define TREE_H

#define MAX_TOWERS_TREE 4
#define MAX_LEVELS      3   // Lv0(기본), Lv1, Lv2

/* 스킬트리 구조체
 * unlocked[타워인덱스][레벨] 형태의 2D 배열로 해금 상태 관리
 * 부모 노드(이전 레벨)가 해금되어야만 자식 노드(다음 레벨) 접근 가능 */
typedef struct {
    int unlocked[MAX_TOWERS_TREE][MAX_LEVELS];
} SkillTree;

void SkillTree_Init(SkillTree *st, int towerCount);              // 스킬트리 초기화 (Lv0 전체 해금)
int  SkillTree_CanUnlock(const SkillTree *st, int towerIdx, int level); // 업그레이드 가능 여부 확인
void SkillTree_Unlock(SkillTree *st, int towerIdx, int level);   // 특정 레벨 해금
void SkillTree_Lock(SkillTree *st, int towerIdx, int level);     // 특정 레벨 잠금 (판매 시 사용)

#endif
