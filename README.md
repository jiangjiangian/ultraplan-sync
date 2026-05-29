[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/VwNt5n7h)
# 2026 OOP Assignment #5

- Due: 2026/5/12 23:59:59
- Author: 111703003 廖經翔
- Contact: <111703003@g.nccu.edu.tw>

---

## 2D Game (1/2)

Obejctive: Building a basic structure of a game and practice Object-Oriented Design

### Description

In this assignment, you are required to complete the basic game sturcture. Raylib has no OOP architecture, so you need to create a simple one by yourself.

### Requirements

1. Design an architecture that decouples UI logics and Data logics
2. Design a class that controls player's moving behavior. (e.g. WASD to move.)
3. Design a tool Class that can generate GameObjects. (Reference: [Factory Pattern](https://medium.com/@antwang/factory-method-in-c-the-right-way-e8c5f015fe39))
4. Draw a UML Class Diagram that shows the architecture of your app briefly.

### Expected Output

After running the program, the following should be displayed on the window:

1. At least one simple GameObject 
2. A movable player GameObject with key control

> You can customize every GameObject's appearance

### Clone the Dev Template

```console
git clone https://github.com/oopnccucs/raylib-dev-template.git
```

> If you name your project `OOPFinal`, enter

```console
mv raylib-dev-temlate OOPFinal && cd OOPFinal && rm -rf .git/
```

### Compile and Run

Before compiling the program, in **Project Folder**:

```console
cmake -B build
```

```console
cmake --build build
```

To run the program:

```console
./build/[your_project_name]
```
### Submit the assignment
Please Create a Github Repository of your own, and submit it's URL on Moodle.

### Scoring Criteria

| Description                               | Score |
| ----------------------------------------- | ----- |
| Submit the assignment                     | 20    |
| Able to compile                           | 20    |
| Draw GameObject correctly                 | 20    |
| UI / Data duty separation                 | 10    |
| Implement Player GameObject correctly     | 10    |
| Implement GameObjectFactory               | 10    |
| UML Class Diagram                         | 10    |


### Reminder

1. The default `include` and `resources` path is configured. You don't need to add prefix to locate files. (Just use `#include "some_header.h"` under include/)
2. You may create new files for more features.
3. This assignment is the foundation of the following one.

---

## 專案檔案結構 (Project Structure)

《尋傘記：政大山下篇》— C++20 / Raylib 5.5 俯視角敘事 RPG。原始碼依領域
分成 **10 個 bucket**；`include/`、`src/`、`tests/` 三棵樹平行對應同一組
領域劃分。建置 `cmake -B build && cmake --build build`，測試
`./build/OOP_Raylib_Lab_tests`（369 cases / 5006 assertions）。

```text
ultraplan-sync/
├── README.md                     本檔（作業說明 + 檔案樹）
├── CMakeLists.txt                CMake 建置（GLOB_RECURSE 掃 src/ + tests/）
├── UML.md                        評分項 #7：Mermaid UML / 狀態機 / 循序圖
├── awsome_cpp.md                 C++ 架構審查筆記（SOLID / RAII / EventBus）
├── 遊戲企劃與敘事架構.md           GDD：角色 / karma / 經濟 / 三結局 / 章節脊
├── 系統架構與UML分析：尋傘記.md     系統架構與 UML 分析設計書
│
├── include/                      標頭檔（94；header-only 的 gfx 全在此）
│   ├── gfx/         (18)  Raylib 薄包裝：Camera2D / Texture / Font / Input / Window
│   ├── entities/    (16)  GameObject 階層：Player / NPC / Item / 雨傘家族 / 消耗品
│   ├── world/        (7)  World / Physics / CollisionMask / Buildings / 地形
│   ├── quest/       (11)  章節任務 / ripple / spawn / vendor 配置 / objective
│   ├── state/       (10)  SemesterStateMachine / 各章 State / Interlude / EndingGate
│   ├── dialog/       (6)  DialogLoader / Layout / View / State / Source / Opener
│   ├── vendor/       (5)  Vendor / Config / Loader / Messages / Sprite
│   ├── ui/          (13)  View / HUD / 選單 / 標題 / 角色選擇 / 結局卡 / 無障礙
│   ├── controller/   (7)  GameController / InputHandler / SceneRouter / EventBus / Factory
│   └── harness/      (2)  Harness / ScriptInput（自動遊玩感知層；正常遊玩 inert）
│
├── src/                          實作（43 .cpp + main.cpp，逐 bucket 對應 include/）
│   ├── main.cpp                  組裝根 composition root（刻意留在頂層）
│   ├── entities/(12)  world/(3)  quest/(5)  state/(2)  dialog/(6)
│   └── vendor/(2)  ui/(6)  controller/(5)  harness/(2)   〔gfx header-only，無 .cpp〕
│
├── tests/                        doctest（89 檔，369 cases / 5006 asserts）
│   ├── gfx/(7)  entities/(11)  world/(4)  quest/(23)  state/(5)
│   ├── dialog/(8)  vendor/(5)  ui/(16)  controller/(8)  harness/(3)
│   └── fixtures/                 測試資料（非 .cpp）
│
├── docs/
│   ├── content/                  執行期載入的劇情（DialogLoader 解析）
│   │     chapter1–4 · ending_a/b/c · interlude_market · voice_bible
│   ├── kb/                       2D RPG / Raylib 知識庫索引
│   ├── archive/                  歷史歸檔（僅供回溯，非當前事實）
│   │   ├── cycle9-audit/         UX / 無障礙診斷（accessibility-audit 被測試引用）
│   │   └── iteration-history/    ACCEPTANCE / BUGLEDGER / CHANGELOG 公開鏡像
│   ├── ROADMAP.md · SCRIPT_HANDOFF.md · FILE_ORGANIZATION.md
│   └── SOLID_REVIEW.md · STRICT_REVIEW.md · STRICT_REVIEW_R3.md
│
├── resources/                    執行期資產（版控遊戲實際載入的子集）
│   └── assets/  sprites/（角色） · maps/（地圖遮罩） · style/ · buildings_*_trimmed/
│
└── tools/                        資產 / 地圖管線（Python，本地生成用）
      tiled_to_world.py · composite_worldmap.py · trim_tiles.py · trim_3d.py …
```

> 檔案樹只列會被 clone 取得的版控內容。每個 bucket 內另附 `README.md`
> 說明該領域職責與相依方向（`include/<bucket>/README.md` 等共 30 份）。
> 完整分層理由見 [`docs/FILE_ORGANIZATION.md`](docs/FILE_ORGANIZATION.md)。
