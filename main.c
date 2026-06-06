#include "raylib.h"
#include "queue.h"
#include "priority_queue.h"
#include "tree.h"
#include "sell_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCREEN_W      1280
#define SCREEN_H      720
#define MAX_ENEMIES   64
#define PATH_LEN      8
#define MAX_TOWERS    4
#define WAVE_COUNT    5
#define MAX_PROJECTILES 128
#define MAX_FLOATTEXTS  64

typedef enum { SCENE_MENU, SCENE_GAME, SCENE_WIN, SCENE_LOSE } Scene;

/* 골드 획득 플로팅 텍스트 */
typedef struct {
    float x, y;       /* 현재 위치 */
    float timer;      /* 남은 수명 (초) */
    float maxTimer;   /* 최대 수명 */
    int   amount;     /* 골드 양 */
    int   isBoss;     /* 보스 처치 여부 (크기·색 분기용) */
    int   alive;
} FloatText;

struct Enemy {
    float x, y;
    float hp, maxHp;
    float speed;
    int   pathIndex;
    int   alive;
    int   isBoss;   // 0: 일반, 1: 보스
};

struct Tower {
    float x, y;
    float range;
    float damage;
    float fireRate;
    float fireTimer;
    int   level;
};

struct Projectile {
    Vector2 pos;
    Vector2 targetPos;
    int targetEnemyIdx;
    float speed;
    int level;
    int alive;
};

static Vector2 PATH[PATH_LEN] = {
    {  50, 360 }, { 200, 360 }, { 200, 180 },
    { 500, 180 }, { 500, 540 }, { 800, 540 },
    { 800, 260 }, {1230, 260 }
};

static struct Tower towers[MAX_TOWERS] = {
    { 350, 280 },
    { 650, 430 },
    { 350, 450 },
    { 900, 370 },
};

static Scene        scene = SCENE_MENU;
static struct Enemy enemies[MAX_ENEMIES];
static int          enemyCount = 0;
static int          currentWave = 0;
static int          gold = 100;
static int          lives = 3;
static float        waveTimer = 0.0f;
static int          selectedTower = 0;

static int          showPopupMenu = 0;
static Vector2      popupMenuPos = { 0, 0 };
static int          popupTowerIdx = -1;
static Rectangle    btnUpgradeRec = { 0, 0, 110, 30 };
static Rectangle    btnSellRec = { 0, 0, 110, 30 };

static struct Projectile projectiles[MAX_PROJECTILES];
static FloatText         floatTexts[MAX_FLOATTEXTS];

// 오디오 리소스 변수
static Sound     monsterDieSound;
static Sound     bossDieSound;
static Sound     towerAttackSound0;
static Sound     towerAttackSound1;
static Sound     towerAttackSound2;
static Sound     towerUpgradeSound;
static Music     menuMusic;
static Music     gameMusic;
static Music     bossMusic;

static Queue         waveQueue;
static PriorityQueue targetPQ;
static SkillTree     skillTree;
static SellQueue     sellQueue;

/* ── 전방 선언 ── */
static void DrawHPBar(float x, float y, float hp, float maxHp);
static void UpdateDrawGame(void);
static void SpawnFloatText(float x, float y, int amount, int isBoss);

/* 두 점 사이의 유클리드 거리 계산
 * 타워-적 거리 측정 및 투사체 도달 판정에 사용 */
static float Dist(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return sqrtf(dx * dx + dy * dy);
}

/* 적을 경로 시작점에 스폰
 * enemies 배열에서 비어있는 슬롯을 찾아 초기화
 * hp: 체력, speed: 이동속도, isBoss: 보스 여부 */
static void SpawnEnemy(float hp, float speed, int isBoss) {
    int i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) {
            enemies[i].x = PATH[0].x;
            enemies[i].y = PATH[0].y;
            enemies[i].hp = hp;
            enemies[i].maxHp = hp;
            enemies[i].speed = speed;
            enemies[i].pathIndex = 1;
            enemies[i].alive = 1;
            enemies[i].isBoss = isBoss;
            enemyCount++;
            return;
        }
    }
}

/* 투사체를 생성하여 지정된 적을 향해 발사
 * projectiles 배열에서 비어있는 슬롯을 찾아 초기화
 * startPos: 발사 위치, targetIdx: 타겟 적 인덱스, speed: 투사체 속도, lv: 타워 레벨 */
static void SpawnProjectile(Vector2 startPos, int targetIdx, float speed, int lv) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].alive) {
            projectiles[i].pos = startPos;
            projectiles[i].targetEnemyIdx = targetIdx;
            projectiles[i].targetPos = (Vector2){ enemies[targetIdx].x, enemies[targetIdx].y };
            projectiles[i].speed = speed;
            projectiles[i].level = lv;
            projectiles[i].alive = 1;
            return;
        }
    }
}

/* 웨이브 시작 시 적 정보를 waveQueue에 미리 등록
 * 마지막 웨이브(WAVE_COUNT-1)는 보스 1마리, 나머지는 일반 적 (5 + wave*2)마리
 * 게임 루프에서 1초마다 Dequeue하여 순서대로 스폰 */
static void StartWave(int wave) {
    int i;
    EnemyInfo info = { 0 };   /* 미초기화 필드 방지 */
    Queue_Clear(&waveQueue);

    if (wave == WAVE_COUNT - 1) {
        info.hp = 1000.0f;
        info.speed = 60.0f;
        info.isBoss = 1;
        Queue_Enqueue(&waveQueue, info);
    }
    else {
        int waveSize = 5 + wave * 2;
        for (i = 0; i < waveSize; i++) {
            info.hp = 40.0f + wave * 15.0f;
            info.speed = 80.0f + wave * 5.0f;
            info.isBoss = 0;
            Queue_Enqueue(&waveQueue, info);
        }
    }
    waveTimer = 0.0f;
}

/* 매 프레임 적의 이동을 업데이트
 * PATH 배열의 경유지를 순서대로 따라 이동
 * 경로 끝 도달 시: 보스면 즉시 패배(lives=0), 일반 적이면 라이프 1 감소 */
static void UpdateEnemies(float dt) {
    int i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        float dx, dy, dist;
        Vector2 target;
        struct Enemy* e;
        if (!enemies[i].alive) continue;
        e = &enemies[i];

        if (e->pathIndex >= PATH_LEN) {
            e->alive = 0;
            enemyCount--;

            if (currentWave == WAVE_COUNT - 1) {
                lives = 0;
            }
            else {
                lives--;
            }
            continue;
        }

        target = PATH[e->pathIndex];
        dx = target.x - e->x;
        dy = target.y - e->y;
        dist = sqrtf(dx * dx + dy * dy);
        if (dist < 4.0f) {
            e->pathIndex++;
        }
        else {
            e->x += (dx / dist) * e->speed * dt;
            e->y += (dy / dist) * e->speed * dt;
        }
    }
}

/* 매 프레임 타워의 공격을 업데이트
 * PriorityQueue로 사거리 내 적 중 가장 가까운 적을 O(log n)으로 선택
 * 공격 쿨타임(fireTimer)이 0 이하일 때 투사체 발사 */
static void UpdateTowers(float dt) {
    int t, i, targetIdx;
    for (t = 0; t < MAX_TOWERS; t++) {
        struct Tower* tw = &towers[t];
        tw->fireTimer -= dt;
        if (tw->fireTimer > 0) continue;
        PQ_Clear(&targetPQ);
        for (i = 0; i < MAX_ENEMIES; i++) {
            float d;
            if (!enemies[i].alive) continue;
            d = Dist(tw->x, tw->y, enemies[i].x, enemies[i].y);
            if (d <= tw->range) PQ_Insert(&targetPQ, i, d);
        }
        if (PQ_IsEmpty(&targetPQ)) continue;
        targetIdx = PQ_ExtractMin(&targetPQ);

        float projSpeed = 400.0f + (tw->level * 150.0f);
        SpawnProjectile((Vector2) { tw->x, tw->y }, targetIdx, projSpeed, tw->level);

        // 타워 레벨별 공격 효과음 재생
        if (tw->level == 0) PlaySound(towerAttackSound0);
        else if (tw->level == 1) PlaySound(towerAttackSound1);
        else if (tw->level == 2) PlaySound(towerAttackSound2);

        tw->fireTimer = 1.0f / tw->fireRate;
    }
}

/* 매 프레임 투사체의 이동 및 충돌을 업데이트
 * 타겟 적이 살아있으면 현재 위치를 추적, 사망 시 마지막 위치로 이동
 * 적과의 거리 8 이하 시 데미지 적용 및 투사체 소멸
 * 적 사망 시 골드 획득 및 플로팅 텍스트 생성 */
static void UpdateProjectiles(float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].alive) continue;
        struct Projectile* p = &projectiles[i];

        if (enemies[p->targetEnemyIdx].alive) {
            p->targetPos = (Vector2){ enemies[p->targetEnemyIdx].x, enemies[p->targetEnemyIdx].y };
        }

        float dx = p->targetPos.x - p->pos.x;
        float dy = p->targetPos.y - p->pos.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 8.0f) {
            p->alive = 0;
            if (enemies[p->targetEnemyIdx].alive) {
                float dmg = 10.0f + (p->level * 5.0f);
                enemies[p->targetEnemyIdx].hp -= dmg;
                if (enemies[p->targetEnemyIdx].hp <= 0) {
                    float ex = enemies[p->targetEnemyIdx].x;
                    float ey = enemies[p->targetEnemyIdx].y;
                    enemies[p->targetEnemyIdx].alive = 0;
                    enemyCount--;

                    /* 사망 효과음 재생 분기 */
                    if (enemies[p->targetEnemyIdx].isBoss) {
                        PlaySound(bossDieSound);
                        int earned = GetRandomValue(150, 200);
                        gold += earned;
                        SpawnFloatText(ex, ey, earned, 1);  /* 보스 플로팅 텍스트 */
                    }
                    else {
                        PlaySound(monsterDieSound);
                        int earned = GetRandomValue(10, 25);
                        gold += earned;
                        SpawnFloatText(ex, ey, earned, 0);  /* 일반 플로팅 텍스트 */
                    }
                }
            }
        }
        else {
            p->pos.x += (dx / dist) * p->speed * dt;
            p->pos.y += (dy / dist) * p->speed * dt;
        }
    }
}

/* 지정된 타워를 업그레이드
 * 조건: 최대 레벨 미만, SkillTree 해금 가능, 골드 충분
 * 업그레이드 전 스탯을 sellQueue에 Enqueue하여 추후 환불 가능하게 보존
 * 스탯 증가: DMG+5, Range+20, FireRate+0.2 */
static void UpgradeTower(int tIdx) {
    int cost;
    UpgradeInfo info;
    struct Tower* tw = &towers[tIdx];
    if (tw->level >= 2) return;
    if (!SkillTree_CanUnlock(&skillTree, tIdx, tw->level + 1)) return;

    cost = (tw->level == 0) ? 60 : 100;
    if (gold < cost) return;

    info.towerIdx = tIdx;
    info.prevLevel = tw->level;
    info.prevDamage = tw->damage;
    info.prevRange = tw->range;
    info.prevRate = tw->fireRate;
    SellQueue_Enqueue(&sellQueue, info);

    gold -= cost;
    tw->level++;
    tw->damage += 5.0f;
    tw->range += 20.0f;
    tw->fireRate += 0.2f;
    SkillTree_Unlock(&skillTree, tIdx, tw->level);

    // 업그레이드 효과음 재생
    PlaySound(towerUpgradeSound);
}

/* sellQueue의 front에서 업그레이드 이력을 꺼내 환불 처리
 * FIFO 구조로 가장 먼저 업그레이드된 타워(보스가 먼저 지나친 타워)부터 처리
 * 이전 스탯 복원 및 골드 환급, SkillTree 레벨 잠금 */
static void SellUpgrade(void) {
    UpgradeInfo info;
    struct Tower* tw;
    int cost;
    if (SellQueue_IsEmpty(&sellQueue)) return;
    info = SellQueue_Dequeue(&sellQueue);
    tw = &towers[info.towerIdx];

    cost = (info.prevLevel == 0) ? 60 : 100;
    gold += cost;
    tw->level = info.prevLevel;
    tw->damage = info.prevDamage;
    tw->range = info.prevRange;
    tw->fireRate = info.prevRate;
    SkillTree_Lock(&skillTree, info.towerIdx, info.prevLevel + 1);
}

/* 게임 상태 전체 초기화
 * 적/투사체/플로팅텍스트 배열 초기화, 타워 스탯 리셋
 * 자료구조(Queue, PQ, SkillTree, SellQueue) 초기화 후 첫 웨이브 시작 */
static void InitGame(void) {
    int i;
    memset(enemies, 0, sizeof(enemies));
    memset(projectiles, 0, sizeof(projectiles));
    memset(floatTexts, 0, sizeof(floatTexts));
    enemyCount = 0;
    currentWave = 0;
    gold = 100;
    lives = 3;
    showPopupMenu = 0;
    popupTowerIdx = -1;
    for (i = 0; i < MAX_TOWERS; i++) {
        towers[i].level = 0;
        towers[i].damage = 10.0f;
        towers[i].range = 150.0f;
        towers[i].fireRate = 1.0f;
        towers[i].fireTimer = 0.0f;
    }
    Queue_Init(&waveQueue);
    PQ_Init(&targetPQ);
    SkillTree_Init(&skillTree, MAX_TOWERS);
    SellQueue_Init(&sellQueue);
    StartWave(currentWave);

    // 게임 시작 시 음악 전환
    StopMusicStream(menuMusic);
    PlayMusicStream(gameMusic);
}

/* 골드 획득 시 화면에 플로팅 텍스트 생성
 * floatTexts 배열에서 비어있는 슬롯을 찾아 초기화
 * isBoss: 보스 처치 여부 (크기·색·수명 분기) */
static void SpawnFloatText(float x, float y, int amount, int isBoss) {
    for (int i = 0; i < MAX_FLOATTEXTS; i++) {
        if (!floatTexts[i].alive) {
            floatTexts[i].x = x;
            floatTexts[i].y = y - 10.0f; /* 적 머리 바로 위에서 시작 */
            floatTexts[i].timer = isBoss ? 2.0f : 1.4f;
            floatTexts[i].maxTimer = floatTexts[i].timer;
            floatTexts[i].amount = amount;
            floatTexts[i].isBoss = isBoss;
            floatTexts[i].alive = 1;
            return;
        }
    }
}

/* 매 프레임 플로팅 텍스트 위치 및 수명 업데이트
 * 위로 이동하면서 수명이 0이 되면 비활성화 */
static void UpdateFloatTexts(float dt) {
    for (int i = 0; i < MAX_FLOATTEXTS; i++) {
        if (!floatTexts[i].alive) continue;
        floatTexts[i].y -= 55.0f * dt;  /* 위로 올라가는 속도 */
        floatTexts[i].timer -= dt;
        if (floatTexts[i].timer <= 0.0f) floatTexts[i].alive = 0;
    }
}

/* 활성화된 플로팅 텍스트를 화면에 렌더링
 * 수명 비율에 따라 알파값 페이드 아웃
 * 보스 처치: 크고 황금색 그림자 효과 / 일반: 작고 연한 초록-노랑 */
static void DrawFloatTexts(void) {
    for (int i = 0; i < MAX_FLOATTEXTS; i++) {
        if (!floatTexts[i].alive) continue;
        FloatText* ft = &floatTexts[i];

        /* 수명 비율로 알파 페이드 아웃 */
        float ratio = ft->timer / ft->maxTimer;
        unsigned char alpha = (unsigned char)(255 * ratio);

        char buf[16];
        sprintf(buf, "+%dG", ft->amount);

        if (ft->isBoss) {
            /* 보스: 크고 황금색 + 외곽선 효과 */
            int fontSize = 26;
            int tw = MeasureText(buf, fontSize);
            /* 그림자 */
            DrawText(buf, (int)ft->x - tw / 2 + 2, (int)ft->y + 2, fontSize,
                CLITERAL(Color){80, 50, 0, alpha});
            /* 본문 */
            DrawText(buf, (int)ft->x - tw / 2, (int)ft->y, fontSize,
                CLITERAL(Color){255, 215, 0, alpha});
        }
        else {
            /* 일반: 작고 연한 초록-노랑 */
            int fontSize = 18;
            int tw = MeasureText(buf, fontSize);
            DrawText(buf, (int)ft->x - tw / 2, (int)ft->y, fontSize,
                CLITERAL(Color){180, 230, 100, alpha});
        }
    }
}

/* 적의 HP바를 캐릭터 머리 위에 렌더링
 * HP 비율에 따라 색상 변화: 50% 이상 초록, 25~50% 노랑, 25% 미만 빨강 */
static void DrawHPBar(float x, float y, float hp, float maxHp) {
    float ratio = hp / maxHp;
    Color c;
    DrawRectangle((int)x - 15, (int)y - 22, 30, 5, DARKGRAY);
    c = ratio > 0.5f ? GREEN : (ratio > 0.25f ? YELLOW : RED);
    DrawRectangle((int)x - 15, (int)y - 22, (int)(30 * ratio), 5, c);
}

/* 메인 메뉴 화면 업데이트 및 렌더링
 * ENTER 키 입력 시 InitGame() 호출 후 게임 씬으로 전환 */
static void UpdateDrawMenu(void) {
    if (IsKeyPressed(KEY_ENTER)) { InitGame(); scene = SCENE_GAME; }
    BeginDrawing();
    ClearBackground(CLITERAL(Color) { 20, 24, 40, 255 });
    DrawText("TOWER DEFENSE",
        SCREEN_W / 2 - MeasureText("TOWER DEFENSE", 72) / 2, 160, 72,
        CLITERAL(Color){220, 180, 60, 255});
    DrawText("War of Kingdoms",
        SCREEN_W / 2 - MeasureText("War of Kingdoms", 36) / 2, 250, 36,
        CLITERAL(Color){180, 140, 40, 255});
    DrawRectangle(SCREEN_W / 2 - 200, 320, 400, 2, CLITERAL(Color){80, 80, 100, 255});
    DrawText("ENTER - Start Game",
        SCREEN_W / 2 - MeasureText("ENTER - Start Game", 28) / 2, 360, 28, LIGHTGRAY);
    DrawText("Left Click: Select Tower  |  Right Click: Upgrade & Sell Menu",
        SCREEN_W / 2 - MeasureText("Left Click: Select Tower  |  Right Click: Upgrade & Sell Menu", 20) / 2,
        410, 20, GRAY);
    EndDrawing();
}

/* 게임 씬의 메인 루프 함수 - 매 프레임 호출
 * 웨이브 스폰(Dequeue), 패배/승리 체크, 마우스 입력 처리
 * 적·타워·투사체·플로팅텍스트 업데이트 후 전체 렌더링
 * HUD, 팝업 메뉴, 스킬트리 패널 포함 */
static void UpdateDrawGame(void) {
    int i, t, lv;
    float dt = GetFrameTime();

    waveTimer += dt;
    if (waveTimer >= 1.0f && !Queue_IsEmpty(&waveQueue)) {
        EnemyInfo info = Queue_Dequeue(&waveQueue);
        SpawnEnemy(info.hp, info.speed, info.isBoss);
        waveTimer = 0.0f;
    }
    /* 패배 체크를 웨이브 클리어보다 먼저 - 보스 도달 시 즉시 패배 */
    if (lives <= 0) {
        StopMusicStream(gameMusic);
        StopMusicStream(bossMusic);
        scene = SCENE_LOSE;
        return;
    }

    if (Queue_IsEmpty(&waveQueue) && enemyCount == 0) {
        currentWave++;
        if (currentWave >= WAVE_COUNT) {
            StopMusicStream(bossMusic);
            scene = SCENE_WIN;
            return;
        }

        // 보스 웨이브 진입 시 BGM 교체
        if (currentWave == WAVE_COUNT - 1) {
            StopMusicStream(gameMusic);
            PlayMusicStream(bossMusic);
        }
        StartWave(currentWave);
    }

    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int clickedOnPopup = 0;
        if (showPopupMenu) {
            if (CheckCollisionPointRec(mousePos, btnUpgradeRec)) {
                UpgradeTower(popupTowerIdx);
                showPopupMenu = 0;
                clickedOnPopup = 1;
            }
            else if (CheckCollisionPointRec(mousePos, btnSellRec)) {
                if (SellQueue_ContainsTower(&sellQueue, popupTowerIdx)) {
                    SellUpgrade();
                }
                showPopupMenu = 0;
                clickedOnPopup = 1;
            }
        }

        if (!clickedOnPopup) {
            int clickedTower = 0;
            for (t = 0; t < MAX_TOWERS; t++) {
                struct Tower* tw = &towers[t];
                if (Dist(mousePos.x, mousePos.y, tw->x, tw->y) <= 22.0f) {
                    selectedTower = t;
                    clickedTower = 1;
                    showPopupMenu = 0;
                    break;
                }
            }
            if (!clickedTower) showPopupMenu = 0;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        for (t = 0; t < MAX_TOWERS; t++) {
            struct Tower* tw = &towers[t];
            if (Dist(mousePos.x, mousePos.y, tw->x, tw->y) <= 22.0f) {
                selectedTower = t;
                popupTowerIdx = t;
                popupMenuPos = mousePos;

                /* 팝업이 화면 밖으로 나가지 않도록 클램핑 */
                if (popupMenuPos.x + 130 > SCREEN_W) popupMenuPos.x = (float)(SCREEN_W - 130);
                if (popupMenuPos.y + 85 > SCREEN_H) popupMenuPos.y = (float)(SCREEN_H - 85);

                showPopupMenu = 1;
                btnUpgradeRec.x = popupMenuPos.x + 10;
                btnUpgradeRec.y = popupMenuPos.y + 10;
                btnSellRec.x = popupMenuPos.x + 10;
                btnSellRec.y = popupMenuPos.y + 45;
                break;
            }
        }
    }

    UpdateEnemies(dt);
    UpdateTowers(dt);
    UpdateProjectiles(dt);
    UpdateFloatTexts(dt);

    BeginDrawing();
    ClearBackground(CLITERAL(Color) { 30, 36, 50, 255 });

    for (i = 0; i < PATH_LEN - 1; i++)
        DrawLineEx(PATH[i], PATH[i + 1], 30, CLITERAL(Color){60, 50, 30, 200});
    for (i = 0; i < PATH_LEN; i++)
        DrawCircleV(PATH[i], 6, CLITERAL(Color){100, 80, 40, 255});

    for (t = 0; t < MAX_TOWERS; t++) {
        char lvlStr[8];
        Color c = (t == selectedTower) ? YELLOW : CLITERAL(Color) { 80, 160, 220, 255 };
        struct Tower* tw = &towers[t];
        DrawCircle((int)tw->x, (int)tw->y, 22, c);
        DrawCircleLines((int)tw->x, (int)tw->y, (int)tw->range,
            CLITERAL(Color){c.r, c.g, c.b, 40});
        sprintf(lvlStr, "L%d", tw->level);
        DrawText(lvlStr, (int)tw->x - 8, (int)tw->y - 8, 16, WHITE);
    }

    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        int   radius = enemies[i].isBoss ? 24 : 14;
        Color enemyColor = enemies[i].isBoss
            ? CLITERAL(Color) { 160, 20, 20, 255 }
        : CLITERAL(Color) { 200, 60, 60, 255 };
        DrawCircle((int)enemies[i].x, (int)enemies[i].y, radius, enemyColor);
        DrawHPBar(enemies[i].x, enemies[i].y, enemies[i].hp, enemies[i].maxHp);
    }

    for (int pIdx = 0; pIdx < MAX_PROJECTILES; pIdx++) {
        if (!projectiles[pIdx].alive) continue;
        struct Projectile* p = &projectiles[pIdx];

        if (p->level == 0) {
            DrawCircleV(p->pos, 4.0f, ORANGE);
        }
        else if (p->level == 1) {
            DrawCircleV(p->pos, 7.0f, SKYBLUE);
            DrawCircleV(p->pos, 4.0f, WHITE);
        }
        else if (p->level == 2) {
            DrawCircleV(p->pos, 10.0f, PURPLE);
            DrawCircleV(p->pos, 7.0f, PINK);
            DrawCircleV(p->pos, 4.0f, WHITE);
        }
    }

    /* 골드 획득 플로팅 텍스트 */
    DrawFloatTexts();

    if (showPopupMenu) {
        DrawRectangle(popupMenuPos.x + 5, popupMenuPos.y + 5, 120, 75, CLITERAL(Color){25, 30, 45, 245});
        DrawRectangleLines(popupMenuPos.x + 5, popupMenuPos.y + 5, 120, 75, GOLD);

        int upgradeCost = (towers[popupTowerIdx].level == 0) ? 60 : 100;
        int isHoverUpgrade = CheckCollisionPointRec(mousePos, btnUpgradeRec);

        if (towers[popupTowerIdx].level >= 2) {
            DrawRectangleRec(btnUpgradeRec, DARKGRAY);
            DrawText("Lv MAX", btnUpgradeRec.x + 10, btnUpgradeRec.y + 8, 14, GRAY);
        }
        else {
            DrawRectangleRec(btnUpgradeRec, isHoverUpgrade ? GREEN : CLITERAL(Color) { 40, 120, 60, 255 });
            char upStr[32];
            sprintf(upStr, "Upgrade (%dG)", upgradeCost);
            DrawText(upStr, btnUpgradeRec.x + 6, btnUpgradeRec.y + 8, 12, WHITE);
        }

        int isHoverSell = CheckCollisionPointRec(mousePos, btnSellRec);
        int canSell = SellQueue_ContainsTower(&sellQueue, popupTowerIdx);

        if (canSell) {
            DrawRectangleRec(btnSellRec, isHoverSell ? RED : CLITERAL(Color) { 150, 40, 40, 255 });
            char sellStr[32];
            int refund = (sellQueue.data[sellQueue.front].prevLevel == 0) ? 60 : 100;
            sprintf(sellStr, "Sell (+%dG)", refund);
            DrawText(sellStr, btnSellRec.x + 12, btnSellRec.y + 8, 12, WHITE);
        }
        else {
            DrawRectangleRec(btnSellRec, DARKGRAY);
            DrawText("Sell (Locked)", btnSellRec.x + 10, btnSellRec.y + 8, 11, GRAY);
        }
    }

    /* HUD */
    DrawRectangle(0, 0, SCREEN_W, 48, CLITERAL(Color){15, 18, 30, 230});
    {
        char hud[256];
        int nextCost = (towers[selectedTower].level == 0) ? 60 : 100;

        if (currentWave == WAVE_COUNT - 1) {
            if (towers[selectedTower].level >= 2) {
                sprintf(hud, "[BOSS WAVE]  Gold: %d  Lives: %d  Tower[%d] Lv%d (MAX)  [Right-Click Tower for Menu]",
                    gold, lives, selectedTower + 1, towers[selectedTower].level);
            }
            else {
                sprintf(hud, "[BOSS WAVE]  Gold: %d  Lives: %d  Tower[%d] Lv%d (Next: %dG)  [Right-Click Tower for Menu]",
                    gold, lives, selectedTower + 1, towers[selectedTower].level, nextCost);
            }
        }
        else {
            if (towers[selectedTower].level >= 2) {
                sprintf(hud, "Wave: %d/%d  Gold: %d  Lives: %d  Tower[%d] Lv%d (MAX)  [Right-Click Tower for Menu]",
                    currentWave + 1, WAVE_COUNT, gold, lives, selectedTower + 1, towers[selectedTower].level);
            }
            else {
                sprintf(hud, "Wave: %d/%d  Gold: %d  Lives: %d  Tower[%d] Lv%d (Next: %dG)  [Right-Click Tower for Menu]",
                    currentWave + 1, WAVE_COUNT, gold, lives, selectedTower + 1, towers[selectedTower].level, nextCost);
            }
        }
        DrawText(hud, 10, 14, 20, RAYWHITE);
    }

    /* Skill Tree Panel */
    DrawRectangle(SCREEN_W - 250, SCREEN_H - 170, 240, 160, CLITERAL(Color){15, 18, 30, 220});
    DrawText("Skill Tree", SCREEN_W - 238, SCREEN_H - 158, 20, GOLD);
    for (lv = 1; lv <= 2; lv++) {
        char sstr[64];
        int canUnlock = SkillTree_CanUnlock(&skillTree, selectedTower, lv);
        int unlocked = towers[selectedTower].level >= lv;
        Color sc = unlocked ? GREEN : (canUnlock ? YELLOW : DARKGRAY);
        sprintf(sstr, "Lv%d: DMG+5 Range+20 Rate+0.2", lv);
        DrawText(sstr, SCREEN_W - 238, SCREEN_H - 120 + lv * 40, 15, sc);
        DrawText(unlocked ? "[UNLOCKED]" : (canUnlock ? "[AVAILABLE]" : "[LOCKED]"),
            SCREEN_W - 238, SCREEN_H - 104 + lv * 40, 13, sc);
    }

    EndDrawing();
}

/* 게임 종료 화면(승리/패배) 업데이트 및 렌더링
 * win: 1이면 승리 화면, 0이면 패배 화면
 * ENTER 키 입력 시 메뉴 씬으로 복귀 */
static void UpdateDrawEnding(int win) {
    if (IsKeyPressed(KEY_ENTER)) {
        scene = SCENE_MENU;
        PlayMusicStream(menuMusic);
    }
    BeginDrawing();
    ClearBackground(CLITERAL(Color) { 10, 12, 20, 255 });
    if (win) {
        DrawText("VICTORY!",
            SCREEN_W / 2 - MeasureText("VICTORY!", 80) / 2, 240, 80,
            CLITERAL(Color){220, 200, 60, 255});
        DrawText("All waves cleared!",
            SCREEN_W / 2 - MeasureText("All waves cleared!", 30) / 2, 350, 30, LIGHTGRAY);
    }
    else {
        DrawText("DEFEAT",
            SCREEN_W / 2 - MeasureText("DEFEAT", 80) / 2, 240, 80,
            CLITERAL(Color){200, 60, 60, 255});
        DrawText("Your base has fallen...",
            SCREEN_W / 2 - MeasureText("Your base has fallen...", 30) / 2, 350, 30, LIGHTGRAY);
    }
    DrawText("ENTER - Back to Menu",
        SCREEN_W / 2 - MeasureText("ENTER - Back to Menu", 24) / 2, 430, 24, GRAY);
    EndDrawing();
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Tower Defense - War of Kingdoms");

    // 오디오 장치 초기화
    InitAudioDevice();

    // 효과음 파일 로드
    monsterDieSound = LoadSound("assets/monster_die.mp3");
    bossDieSound = LoadSound("assets/boss_die.mp3");
    towerAttackSound0 = LoadSound("assets/tower_attack0.mp3");
    towerAttackSound1 = LoadSound("assets/tower_attack1.mp3");
    towerAttackSound2 = LoadSound("assets/tower_attack2.mp3");
    towerUpgradeSound = LoadSound("assets/tower_upgrade.mp3");

    // 배경음악 파일 로드
    menuMusic = LoadMusicStream("assets/menu_theme.mp3");
    gameMusic = LoadMusicStream("assets/game_theme.mp3");
    bossMusic = LoadMusicStream("assets/boss_theme.mp3");

    PlayMusicStream(menuMusic);

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        // 음악 스트림 실시간 업데이트
        if (scene == SCENE_MENU) UpdateMusicStream(menuMusic);
        else if (scene == SCENE_GAME) {
            if (currentWave == WAVE_COUNT - 1) UpdateMusicStream(bossMusic);
            else UpdateMusicStream(gameMusic);
        }

        switch (scene) {
        case SCENE_MENU: UpdateDrawMenu();    break;
        case SCENE_GAME: UpdateDrawGame();    break;
        case SCENE_WIN:  UpdateDrawEnding(1); break;
        case SCENE_LOSE: UpdateDrawEnding(0); break;
        }
    }

    // 오디오 리소스 해제
    UnloadSound(monsterDieSound);
    UnloadSound(bossDieSound);
    UnloadSound(towerAttackSound0);
    UnloadSound(towerAttackSound1);
    UnloadSound(towerAttackSound2);
    UnloadSound(towerUpgradeSound);

    UnloadMusicStream(menuMusic);
    UnloadMusicStream(gameMusic);
    UnloadMusicStream(bossMusic);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}