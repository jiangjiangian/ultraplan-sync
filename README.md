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

《尋傘記：政大山下篇》— C++20 / Raylib 5.5 俯視角敘事 RPG。原始碼採
**領域分層（domain-layered）**：`include/` 與 `src/` 兩棵樹平行鏡射，
頂層皆切成 **app / engine / game / ui** 四個領域；相依方向為單向
`app → game / ui → engine`（engine 不反向相依 game/ui）。`tests/` 另成一棵
樹，依相同領域子目錄組織。

### 建置與測試

```bash
cmake -B build && cmake --build build      # 設定並編譯（含遊戲與測試）
./build/umbrella                           # 執行遊戲
ctest --test-dir build                     # 跑測試（CTest，目標名 unit_tests）
./build/umbrella_test                      # 或直接執行 doctest 測試二進位
```

CMake 以 `GLOB_RECURSE` 掃描 `src/`、`tests/`，並在建置時把整個
`resources/` 複製到執行檔旁，遊戲以相對工作目錄載入資產。

### 目錄樹

```text
.
├── README.md                             本檔（作業說明 + 檔案樹）
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
├── tests/                                doctest 測試套件（112 個 .cpp，571 個 TEST_CASE）
│   └── （依 domain 分目錄）                  controller / dialog / entities / gfx / harness / quest / state / ui / vendor / world / fixtures
│
├── docs/                                 UML.md（評分用 UML 設計）+ content/（執行期載入的劇情 Markdown）
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
