#include "tree.h"
#include <string.h>

/* 스킬트리 초기화
 * 모든 레벨을 잠금 상태로 초기화한 뒤, Lv0(기본)만 전체 타워에 대해 해금
 * towerCount: 초기화할 타워 수 */
void SkillTree_Init(SkillTree *st, int towerCount) {
    memset(st->unlocked, 0, sizeof(st->unlocked));
    for (int i = 0; i < towerCount; i++)
        st->unlocked[i][0] = 1; // Lv0(기본)은 모두 해금 상태
}

/* 특정 타워의 특정 레벨 업그레이드 가능 여부 확인
 * 조건: 이전 레벨(부모 노드)이 해금되어 있고, 현재 레벨은 미해금 상태여야 함
 * 반환: 가능하면 1, 불가능하면 0 */
int SkillTree_CanUnlock(const SkillTree *st, int towerIdx, int level) {
    if (level <= 0 || level >= MAX_LEVELS) return 0;
    if (st->unlocked[towerIdx][level]) return 0;     // 이미 해금된 레벨
    return st->unlocked[towerIdx][level - 1];        // 부모 노드(이전 레벨) 해금 여부
}

/* 특정 타워의 특정 레벨을 해금 상태로 설정
 * 업그레이드 성공 시 호출 */
void SkillTree_Unlock(SkillTree *st, int towerIdx, int level) {
    if (level < MAX_LEVELS)
        st->unlocked[towerIdx][level] = 1;
}

/* 특정 타워의 특정 레벨을 잠금 상태로 설정
 * 업그레이드 판매(환불) 시 호출하여 트리 상태를 이전으로 복원 */
void SkillTree_Lock(SkillTree *st, int towerIdx, int level) {
    if (level < MAX_LEVELS)
        st->unlocked[towerIdx][level] = 0;
}
