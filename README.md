# Tower Defense - War of Kingdoms

C언어와 Raylib를 활용한 타워 디펜스 게임입니다.  
자료구조(Queue, Priority Queue, Tree, SellQueue)를 게임 시스템에 직접 적용하여 구현하였습니다.

---

## 프로젝트 구조

```
project/
├── main.c
├── queue.c / queue.h
├── sell_queue.c / sell_queue.h
├── priority_queue.c / priority_queue.h
├── tree.c / tree.h
├── assets/
│   ├── menu_theme.mp3
│   ├── game_theme.mp3
│   ├── boss_theme.mp3
│   ├── monster_die.mp3
│   ├── boss_die.mp3
│   ├── tower_attack0.mp3
│   ├── tower_attack1.mp3
│   ├── tower_attack2.mp3
│   └── tower_upgrade.mp3
└── README.md
```

---

## 사용된 자료구조

| 자료구조                  | 파일                                | 적용 위치                 |
| ------------------------- | ----------------------------------- | ------------------------- |
| Queue (원형 큐)           | queue.c / queue.h                   | 웨이브 적 스폰 순서 관리  |
| SellQueue (원형 큐)       | sell_queue.c / sell_queue.h         | 업그레이드 판매 순서 관리 |
| Priority Queue (Min-Heap) | priority_queue.c / priority_queue.h | 타워 타겟 선정            |
| Skill Tree (2D 배열)      | tree.c / tree.h                     | 업그레이드 해금 조건 관리 |

---

## 자료구조 적용 설명

### Queue — 웨이브 스폰

웨이브 시작 시 적 정보를 미리 `Enqueue`해두고, 게임 루프에서 1초마다 `Dequeue`하여 순서대로 스폰합니다. FIFO 구조로 등록된 순서대로 적이 등장합니다.

### SellQueue — 업그레이드 판매

업그레이드 시 이전 스탯을 `Enqueue`합니다. 보스 경로 특성상 먼저 업그레이드한 타워(보스가 먼저 지나친 타워)부터 판매 가능하도록 FIFO로 제한합니다. 판매 시 골드 환급 및 스탯 복원이 이루어집니다.

### Priority Queue — 타겟 선정

매 프레임 사거리 내 모든 적을 거리값과 함께 삽입하고, `ExtractMin`으로 가장 가까운 적을 O(log n)으로 추출하여 타겟으로 설정합니다.

### Skill Tree — 업그레이드 해금

`unlocked[타워인덱스][레벨]` 2D 배열로 해금 상태를 관리합니다. 이전 레벨(부모 노드)이 해금된 경우에만 다음 레벨 업그레이드가 가능합니다.

---

## 실행 방법

1. [Raylib](https://www.raylib.com/) 설치
2. Visual Studio에서 프로젝트 열기
3. 소스 파일 전체 추가 (main.c, queue.c, sell_queue.c, priority_queue.c, tree.c)
4. `assets/` 폴더를 실행 파일과 같은 경로에 위치
5. 빌드 후 실행

---

## 게임 조작법

| 입력          | 동작                        |
| ------------- | --------------------------- |
| `ENTER`       | 게임 시작 / 메뉴로 복귀     |
| 좌클릭 (타워) | 타워 선택                   |
| 우클릭 (타워) | 업그레이드 / 판매 메뉴 열기 |

---

## 게임 규칙

- 웨이브는 총 5개이며 마지막 웨이브는 보스 웨이브
- 적이 경로 끝에 도달하면 라이프 감소 (보스 도달 시 즉시 패배)
- 타워 업그레이드: Lv0→Lv1 60G / Lv1→Lv2 100G
- 판매는 가장 먼저 업그레이드한 타워부터 순서대로 가능

---

## 역할 분담

| 팀원   | 담당                                                                                     |
| ------ | ---------------------------------------------------------------------------------------- |
| 최무찬 | Queue / Priority Queue 구현, 웨이브 생성 및 타겟 선정 로직, 전투 시뮬레이션              |
| 백종윤 | Tree / SellQueue 구현, 스킬트리 UI 및 업그레이드 판매 시스템, 승패 판정 및 raylib 렌더링 |

---

## 개발 환경

- 언어: C (C99)
- 라이브러리: Raylib 5.x
- IDE: Visual Studio 2022
- OS: Windows 10 / 11
