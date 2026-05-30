[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/VwNt5n7h)

# 尋傘記：政大山下篇

> 一學期、一把傘、一張雨量表 —— 在政大校園裡，努力不被淋成落湯雞。

## 目錄

- [遊戲簡介](#遊戲簡介)
- [遊戲特色與玩法](#遊戲特色與玩法)
- [建置與執行](#建置與執行)
- [執行測試](#執行測試)
- [專案結構](#專案結構)
- [架構與文件](#架構與文件)
- [課程資訊](#課程資訊)

## 遊戲簡介

《尋傘記：政大山下篇》是一款以 C++20 與 Raylib 5.5 打造的俯視視角敘事 RPG。
玩家扮演的學生橫跨一整個學期、四個章節（加退選之亂、期中考、校慶運動會、
期末考終焉），在政大校園裡奔走求生，目標只有一個：別被雨淋透。

遊戲的核心張力來自兩條交織的數值線。一張 **雨量表** 實質上就是角色的生命值，
淋雨會讓它逼近爆表；同時一套 **業力（karma）** 系統默默記錄你的善意與冷漠，
連同每章一輪的小型 **循環經濟**（賺取與花費），共同決定你會走向哪一種結局。

## 遊戲特色與玩法

- **四種結局**：A 雨過天晴（善終／高業力＋真傘）、B 失溫（雨量表爆表）、
  C 破財消災（集英樓買固定 100 元的醜傘）、D 風雨同行（體諒助教但未達 A），
  判定優先序為 A → B → D → C。
- **雨量表即 HP**：雨量表爆表等同失溫倒下（結局 B），躲雨與撐傘是生存關鍵。
- **業力與詛咒污點**：善行累積高業力，惡行則留下詛咒污點，影響真傘與善終路線。
- **循環經濟**：每章自成一輪「賺取 → 花費」，資源調度直接牽動結局走向。
- **旗標驅動任務**：任務透過 quest hooks 旗標表觸發與推進，章節環環相扣。

## 建置與執行

使用 CMake 建置。CMake 以 `GLOB_RECURSE` 掃描 `src/` 與 `tests/`，並在建置時把
整個 `resources/` 複製到執行檔旁，遊戲以相對工作目錄載入資產。產出的執行檔為
遊戲本體 `./build/umbrella`。

```bash
cmake -B build
cmake --build build
./build/umbrella
```

## 執行測試

測試以 doctest 撰寫，目前共 569 個案例，全數通過。可直接執行測試二進位，或透過
CTest 觸發（CTest 目標名為 `unit_tests`）。

```bash
./build/umbrella_test       # 直接執行 doctest 測試二進位
ctest --test-dir build      # 或透過 CTest 執行
```

## 專案結構

原始碼採 **領域分層（domain-layered）**：`include/` 與 `src/` 兩棵樹平行鏡射，
頂層皆切成 app / engine / game / ui 四個領域；相依方向為單向
`app → game / ui → engine`（engine 不反向相依 game/ui）。`tests/` 另成一棵樹，
依相同領域子目錄組織。

```text
.
├── README.md                             本檔（專案說明 + 作業規格 + 檔案樹）
├── CMakeLists.txt                        CMake 建置（GLOB_RECURSE 掃 src/ + tests/，複製 resources/）
├── CREDITS.md                            第三方素材與字型出處
├── Doxyfile                              Doxygen API 文件設定
├── 遊戲企劃與敘事架構.md                 GDD：角色 / karma / 經濟 / 四結局 / 章節脊
│
├── include/                              標頭檔（144）— 領域分層 app / engine / game / ui
│   ├── app/      (7)                         程式進入點與場景切換（IScene / SceneManager / scenes）
│   ├── engine/   (27)                        引擎層：audio / core / events / input / math / platform / render
│   ├── game/     (89)                        遊戲邏輯：controller / dialog / entities / gfx / quest / state / vendor / world
│   └── ui/       (21)                        View 與 hud / overlay / world，加上扁平視圖（標題 / 角色選擇 / 結局…）
│
├── src/                                  實作（80）— 逐檔對應 include/，相同四領域劃分
│   ├── app/      (7)                         main.cpp（composition root）+ SceneBootstrap / SceneManager / scenes
│   ├── engine/   (8)                         有狀態的引擎實作：audio / events / platform / render（其餘為 header-only）
│   ├── game/     (50)                        controller / dialog / entities / quest / state / vendor / world 的實作
│   └── ui/       (15)                        View 與 hud / overlay / world 視圖實作
│
├── tests/                                doctest 測試套件（依 domain 分目錄）
│   └── （依 domain 分目錄）                  controller / dialog / entities / gfx / harness / quest / state / ui / vendor / world / fixtures
│
├── docs/                                 UML/（UML 設計文件）+ Report.md（口頭報告腳本）+ content/（執行期載入的劇情 Markdown）
│
├── resources/                            執行期資產（CMake 複製到執行檔旁）
│   └── assets/                               sprites / buildings_3d_trimmed / Pixel Art Vending Machines Pack /
│                                             decorations / fonts(cjk.ttf) / maps（碰撞遮罩 + worldmap）
│
└── tools/                                資產 / 地圖管線（Python，本地生成用，非執行期相依）
    └── *.py                                  tiled_to_world / composite_worldmap / trim_tiles / trim_3d …（8 支）
```

> 每個頂層資料夾下皆有一份 `README.md` 說明其內容與職責
> （`include/`、`src/`、`tests/`、`docs/`、`tools/`、`resources/`）。

## 架構與文件

整體採 **MVC** 搭配每幀模擬管線（每個系統實作 `ISystem` 介面），以 **EventBus**
（Observer 模式）解耦事件，並透過 quest hooks 旗標表驅動任務流程。

- [UML 設計文件](docs/UML/README.md) —— 完整架構圖與設計說明（索引含分層、實體、
  狀態機、MVC／ISystem、GoF、SOLID 等章節）。
- [口頭報告腳本](docs/Report.md) —— 課堂口頭報告講稿。

## 課程資訊

本專案為 GitHub Classroom 課程作業。以下保留原始作業規格與繳交說明。

- **Due:** 2026/6/9 23:59:59
- **Author:** jiangjiangian
- **Contact:** <iansu0924@gmail.com>

<details>
<summary>課程作業規格</summary>

### 2026 OOP Assignment #5

#### 2D Game (1/2)

Obejctive: Building a basic structure of a game and practice Object-Oriented
Design

#### Description

In this assignment, you are required to complete the basic game sturcture.
Raylib has no OOP architecture, so you need to create a simple one by yourself.

#### Requirements

1. Design an architecture that decouples UI logics and Data logics
2. Design a class that controls player's moving behavior. (e.g. WASD to move.)
3. Design a tool Class that can generate GameObjects. (Reference:
   [Factory Pattern](https://medium.com/@antwang/factory-method-in-c-the-right-way-e8c5f015fe39))
4. Draw a UML Class Diagram that shows the architecture of your app briefly.

#### Expected Output

After running the program, the following should be displayed on the window:

1. At least one simple GameObject
2. A movable player GameObject with key control

> You can customize every GameObject's appearance

#### Clone the Dev Template

```console
git clone https://github.com/oopnccucs/raylib-dev-template.git
```

> If you name your project `OOPFinal`, enter

```console
mv raylib-dev-temlate OOPFinal && cd OOPFinal && rm -rf .git/
```

#### Compile and Run

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

#### Submit the assignment

Please Create a Github Repository of your own, and submit it's URL on Moodle.

#### Scoring Criteria

| Description | Score |
| --- | --- |
| Submit the assignment | 20 |
| Able to compile | 20 |
| Draw GameObject correctly | 20 |
| UI / Data duty separation | 10 |
| Implement Player GameObject correctly | 10 |
| Implement GameObjectFactory | 10 |
| UML Class Diagram | 10 |

#### Reminder

1. The default `include` and `resources` path is configured. You don't need to
   add prefix to locate files. (Just use `#include "some_header.h"` under
   include/)
2. You may create new files for more features.
3. This assignment is the foundation of the following one.

</details>
