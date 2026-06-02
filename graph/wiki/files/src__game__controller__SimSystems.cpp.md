---
id: "file:src/game/controller/SimSystems.cpp"
type: source
path: src/game/controller/SimSystems.cpp
domain: game
bucket: controller
loc: 112
classes: []
sources: ["src/game/controller/SimSystems.cpp"]
---
# `SimSystems.cpp`

> **一句定位**：每幀模擬管線的四個 ISystem 與終端 SweepSystem 的完整實作：SurvivalSystem（降雨）、MovementSystem（物件 tick）、CollisionSystem（AABB + 地形）、SpawnSystem（圈速 + 延後生成）。

## 職責

此檔案實作五個 `ISystem` 子類別，對應 `GameController` 的 `advanceSystems_` 管線：

**SurvivalSystem::Run**：每幀 tick 玩家降雨生存迴圈。三種狀態：`CurrentBuildingName()` 非空（室內）→ `DrainRain`（完全回復）；室外有傘 → `ApplyRainSheltered(lethal=true)`（+1.5/s）；室外無傘 → `ApplyRain(lethal=true)`（+5/s）。在市集（`Interlude_Market`）與三個結局狀態下略過（非遊戲性狀態）。

**MovementSystem::Run**：儲存玩家 tick 前座標至 `ctx.prevPlayerPos`，再以 `ForEachRole<IUpdatable>` 呼叫所有帶 `IUpdatable` 角色的 `Update(dt)`（編譯期靜態分派，無 Update 的物件不造訪）。前座標透過 `SimContext` 傳給 CollisionSystem。

**CollisionSystem::Run**：先以 `ClampToWorld` 夾限玩家位置（僅有差異才寫入，維持決定性）；再走訪 `BlocksMovement()` 為真的物件建立 `ctx.frameColliders`（玩家大小的 AABB）；最後呼叫 `physics::ResolveMove(prevPlayerPos, currentPos, playerSize, frameColliders, &terrainMask)` 做分軸碰撞解算，位置有差異才寫入。道具刻意不加入 frameColliders（允許玩家穿越拾取）。

**SpawnSystem::Run**：呼叫 `world.UpdateSportsLap()`（Ch3 圈速計時）；然後 `MaybeSpawnChapter1VictimUmbrella()`、`MaybeSpawnChapter2Notes()`、`MaybeSpawnChapter3Umbrella()`、`MaybeSpawnInterludeLibrarianReturn()` — 四個延後生成函式，均在各自章節之外是廉價 no-op，每次只觸發一次。

**SweepSystem::Run**：呼叫 `ctx.world.Sweep()` 執行幀末的 mark-then-sweep 刪除（`isActive_=false` 標記的物件在此回收，不在迭代中 delete）。

## 關鍵內容（類別 / 函式 / 資料）

- `SurvivalSystem::Run` — 三態降雨：室內回復 / 持傘減緩 / 曝露致命；市集 + 結局略過。
- `MovementSystem::Run` — 記錄 prevPlayerPos；`ForEachRole<IUpdatable>` 編譯期分派。
- `CollisionSystem::Run` — 世界 AABB 夾限 + BlocksMovement 碰撞體收集 + `ResolveMove` 分軸解算。
- `SpawnSystem::Run` — `UpdateSportsLap` + 四個 MaybeSpawn 函式（各自守門）。
- `SweepSystem::Run` — `world.Sweep()` 幀末延遲刪除。

## 相依與在架構中的位置
- **#include（往外）**：`SimSystem.h`（ISystem 介面）；`World.h`、`Player.h`（Model）；`Roles.h`（`ForEachRole<IUpdatable>`）；`GameObjectQueries.h`（`ForEachActiveExcept`）；`Physics.h`（`ResolveMove`）；`Bounds.h`（`ClampToWorld`）
- **被誰使用（往內）**：—（五個 ISystem 實例由 `GameController` 建構子建立）
- **繼承 / 實作 / 體現**：實作 `ISystem` 介面（`Run(SimContext&, float)`）；`ForEachRole<IUpdatable>` 用到 CRTP mixin [WithRoles](../concepts/oo-crtp.md)
- **每幀管線 / MVC 角色**：game / controller 層；構成 Survival→Movement→Collision→Spawn 的四個管線階段；SweepSystem 為終端階段（在互動/閘門之後單獨執行）

## OO 概念與設計重點

五個 ISystem 是 [Strategy](../concepts/pat-strategy.md) 模式的應用：每個系統封裝一個模擬職責，可獨立替換或測試。管線次序（Survival→移動→碰撞→生成→Sweep）在 `GameController` 建構子一次確立，此檔案只負責實作。[CRTP](../concepts/oo-crtp.md) mixin `WithRoles<Derived,Base>` 透過 `ForEachRole<IUpdatable>` 在編譯期靜態分派，無需 `dynamic_cast`（[ISP](../concepts/oo-isp-roles.md)）。`CollisionSystem` 的「僅差異才寫入」設計保持浮點決定性，是 harness 逐位元一致性的底層保障。`SpawnSystem` 的四個 `MaybeSpawn` 各自守門且冪等，每幀無條件呼叫是安全的——體現 [arch-isystem](../concepts/arch-isystem.md) 的純模型變動原則。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/SimSystems.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/SimSystems.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[ISystem](../concepts/arch-isystem.md) · [Strategy](../concepts/pat-strategy.md) · [CRTP](../concepts/oo-crtp.md) · [ISP Roles](../concepts/oo-isp-roles.md)
