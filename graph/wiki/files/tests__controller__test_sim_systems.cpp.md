---
id: "file:tests/controller/test_sim_systems.cpp"
type: test
path: tests/controller/test_sim_systems.cpp
domain: tests
bucket: controller
loc: 198
classes: [Fixture]
sources: ["tests/controller/test_sim_systems.cpp"]
---
# `test_sim_systems.cpp`

> **一句定位**：隔離驗證每幀管線中各 `ISystem` 子類別（SurvivalSystem、MovementSystem、CollisionSystem、SpawnSystem、SweepSystem）的單一職責，以 `Fixture` 建立 headless `World` 並直接驅動各 System。

## 職責

本檔為 `GameController::Update` 管線中從「大塊 Update」抽出的各 `ISystem` 提供獨立的單元測試。由於各 System 只操作 `World&` 與 `Player&`，完全無輸入、無 raylib，故以 `loadSprites=false` 的 headless `World` 直接驅動，不需任何 GL。

**`Fixture` struct**：包含 `World{"", false}` 與 `colliders` 向量，提供 `ctx()` 方法建構 `SimContext{w, Vec2{2048,2048}, Vec2{24,24}, colliders, {}}`，對應 `GameController` 每格的建構方式。

**SurvivalSystem**（4 個 case）：
- 室外無傘：1 秒後 `GetRainMeter() > 0`（+5 u/s）。
- 室外有傘：雨量計增加但 `< 5`（遮蔽後的慢速累積）。
- 室內（`CurrentBuildingName() = "圖書館"`）：先灌 2 秒雨量再進室內 0.5 秒，雨量計減少（-10 u/s 回復）。
- `Interlude_Market`：FSM 在市集時 System 為完全 no-op，`GetRainMeter()` 不變。

**MovementSystem**（1 個 case）：驗證 `Run` 後 `ctx.prevPlayerPos` 被設為推進前的玩家位置（`{123, 456}`），確認前一格位置擷取正確。

**CollisionSystem**（1 個 case）：玩家放到 `{9000, 9000}` 世界外，`Run` 後位置被夾回 `[0, 2048-24]²`，驗證四軸夾限。

**SpawnSystem**（1 個 case）：全新 Ch1（無旗標），`Run` 後 `Objects().size()` 不變，驗證「在非對應章節為廉價 no-op」且不崩潰。

**SweepSystem / World::Sweep**（3 個 case）：
- 手動插入一個已 `Deactivate()` 的 NPC 再 `sys.Run`，驗證 `Objects().size()` 少一、`Objects().front()` 仍是 `Player`、`GetPlayer()` 快取仍有效。
- `Player` 死亡時 `Sweep()` 清除快取（`GetPlayer() == nullptr`）且物件數減一；以 `uintptr_t` 比較確認無倖存物件使用舊位址。
- 無物件死亡時 `Sweep()` 為 no-op，`GetPlayer()` 不變。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Fixture`：`World + colliders + ctx()`，對應 `GameController` 每格的 `SimContext` 建構。
- `SurvivalSystem`、`MovementSystem`、`CollisionSystem`、`SpawnSystem`、`SweepSystem`（被測型別，均繼承 `ISystem`）。
- `SimContext`：包含 `World&`、世界尺寸、玩家尺寸、碰撞器清單、前一格玩家位置。
- `World::Sweep()`：mark-then-sweep，移除 `isActive_=false` 物件並維護 `Player` 快取。
- `Player::Deactivate()`、`NPC::Deactivate()`：標記為非作用中，等待幀末 Sweep 清除。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/controller/SimSystem.h`、`include/game/world/World.h`、`include/game/entities/Player.h`、`include/game/entities/NPC.h`、`include/engine/math/Vec2.h`、`include/engine/math/Rect.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：直接測試 Survival → Movement → Collision → Spawn → Sweep 各階段，對應 `GameController::Update` 的完整管線

## OO 概念與設計重點

本檔體現 [arch-isystem](../concepts/arch-isystem.md) 設計的可測試性：每個 `ISystem` 子類別都可在無完整 `GameController` 的情況下被獨立驗證，因為 `SimContext` 是純資料傳遞，不藏任何全域狀態。`Sweep` 的 UAF 測試（以 `uintptr_t` 比較）避免碰觸已釋放指標，是正確的 C++ 記憶體安全實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_sim_systems.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_sim_systems.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-isystem](../concepts/arch-isystem.md)
