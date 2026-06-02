---
id: "file:include/game/controller/SimSystem.h"
type: header
path: include/game/controller/SimSystem.h
domain: game
bucket: controller
loc: 113
classes: [SimContext, ISystem, SurvivalSystem, MovementSystem, CollisionSystem, SpawnSystem, SweepSystem]
sources: ["include/game/controller/SimSystem.h"]
---
# `SimSystem.h`

> **一句定位**：每幀模擬管線的完整定義——`ISystem` 介面、共用 `SimContext`，以及依管線順序排列的五個具體階段（Survival/Movement/Collision/Spawn/Sweep），將原本的 god-method 分解為各自單一職責的階段。

## 職責

`SimSystem.h` 定義了 game controller 層的模擬管線架構。它解決的問題是：`GameController::Update()` 原本把每個模擬階段內嵌為一個龐大的 god-method；現在每個空間上獨立的階段被拆成一個繼承自 `ISystem` 的 `final` 類別，由 Controller 以完全相同的順序執行，確保確定性的序列化輸出逐位元相同。

**MVC 純度紅線**：所有 `ISystem` 的 `Run()` 方法只操作 Model（`World`/`Player`），不讀輸入裝置、不呼叫 raylib、不渲染。各畫面的輸入區塊（結局選單/暫停/對話推進/背包）和 E 鍵互動派發「不是」system，因其讀取輸入，留在 Controller 層。

**`SimContext`**：一次性情境物件，打包一幀所需的所有共用資料：`World&`、`worldSize`、`playerSize`、重用的暫存碰撞盒 vector（`frameColliders`），以及由 `MovementSystem` 設定、`CollisionSystem` 讀取的 `prevPlayerPos`（前一幀玩家座標，用於分軸碰撞解算）。無 raylib、無輸入——純資料橋接。

五個階段依管線順序：
1. **`SurvivalSystem`**：降雨生存。三種狀態分支：在建築內 (`DrainRain` 每秒-10)、室外持傘 (`ApplyRainSheltered` 每秒+1.5)、室外無傘 (`ApplyRain` 每秒+5)。市集/結局狀態下略過。
2. **`MovementSystem`**：存 `prevPlayerPos` 後 tick 所有 `IUpdatable` 物件。
3. **`CollisionSystem`**：玩家 AABB 解算——夾限世界邊界，對 `BlocksMovement()` 物件與地形遮罩做分軸解算。道具刻意不視為碰撞體。
4. **`SpawnSystem`**：延遲生成——tick 操場繞圈進度，再跑四個自我守門的 `MaybeSpawn`（Ch1 苦主之傘/Ch2 筆記/Ch3 雨傘/市集管理員歸還）。
5. **`SweepSystem`**（終端）：幀末標記後清除，委派給 `World::Sweep()`。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `struct SimContext` | 一幀的共用情境；`world`, `worldSize`, `playerSize`, `frameColliders`, `prevPlayerPos`。 |
| `struct ISystem` | 純虛基底；`virtual void Run(SimContext&, float dt) = 0`；`virtual ~ISystem`。 |
| `struct SurvivalSystem final` | 降雨生存累積/回復；三狀態分支。 |
| `struct MovementSystem final` | 存前幀座標 + tick 所有 IUpdatable。 |
| `struct CollisionSystem final` | 玩家 AABB 夾限 + 分軸解算；地形遮罩 + BlocksMovement 物件。 |
| `struct SpawnSystem final` | 繞圈計時 + 四個自我守門 MaybeSpawn。 |
| `struct SweepSystem final` | 委派 `World::Sweep()`；幀末終端階段。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Rect.h`（碰撞盒型別）、`include/engine/math/Vec2.h`（座標/尺寸型別）。
- **被誰使用（往內）**：`include/game/controller/GameController.h`（持有 `vector<unique_ptr<ISystem>> advanceSystems_` 和 `SweepSystem sweep_`）、`src/game/controller/GameController.cpp`（建構各 System 並放入 vector）、`src/game/controller/SimSystems.cpp`（各 System 的 `Run()` 實作）、`tests/controller/test_sim_systems.cpp`（測試各階段）。
- **繼承 / 實作 / 體現**：體現 [ISystem 模擬管線（arch-isystem）](../concepts/arch-isystem.md) 和 [Strategy / Pipeline（pat-strategy）](../concepts/pat-strategy.md)。
- **每幀管線 / MVC 角色**：**每幀管線的定義層**：Survival → Movement → Collision → Spawn（由 `advanceSystems_` 驅動）→ Sweep（`sweep_` 單獨執行）。位於管線中的 Model 操作層，不參與輸入讀取或渲染。

## OO 概念與設計重點

`ISystem` + `SimContext` 是 [ISystem 模擬管線（arch-isystem）](../concepts/arch-isystem.md) 的核心，也是 [Strategy 模式（pat-strategy）](../concepts/pat-strategy.md) 在遊戲 ECS 風格管線上的應用：每個階段以相同介面執行，`GameController` 以 `vector<unique_ptr<ISystem>>` 持有管線，可在不修改 Controller 的情況下插入新階段。

`SweepSystem` 與 `advanceSystems_` 分離（在 `GameController` 中以獨立成員持有）是一個精確的設計決策：Sweep 必須在互動/gate 邏輯「之後」執行，而 `advanceSystems_` 在它們「之前」執行——把 Sweep 放在同一 vector 中並插入尾端無法表達此時序約束，分離成員讓意圖更清晰。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/SimSystem.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SimSystem.h) · [← 全檔索引](../files-index.md) · 相關概念：[ISystem](../concepts/arch-isystem.md) · [Strategy](../concepts/pat-strategy.md)
