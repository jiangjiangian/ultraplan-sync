---
id: "file:include/game/controller/GameController.h"
type: header
path: include/game/controller/GameController.h
domain: game
bucket: controller
loc: 105
classes: [GameController]
sources: ["include/game/controller/GameController.h"]
---
# `GameController.h`

> **一句定位**：MVC 的 Controller 核心——每幀串起輸入處理、`ISystem` 模擬管線、EventBus 訂閱，把所有副作用歸入 `World`，自身不渲染也不擁有 World。

## 職責

`GameController` 是整個遊戲每幀推進的協調者（Orchestrator），屬於 game controller 層，是 MVC 架構中的 C（Controller）。它的職責精確：**收輸入 → 凍結畫面處理 → 推進 ISystem 管線 → 派發互動 → 觸發建築進入 → 幀末清除**，一切結果都寫入注入的 `World&`，不渲染、不擁有世界。

`Update()` 的幀內順序為：
1. `SettleSideEffects`（場景路由副作用：玩家重新定位、消耗品清空等）
2. `HandleEndingMenu()`：若處於 `Ending_*` 狀態，凍結並回傳（結局選單獨占幀）
3. `HandlePauseMenu()`：M 暫停選單（含說明覆蓋層）
4. `HandleDialog()`：對話推進，凍結模擬
5. `HandleInventory()`：Tab 背包覆蓋層
6. ISystem 管線（`advanceSystems_`）：Survival → Movement → Collision → Spawn
7. `DispatchInteract()`：E 鍵互動派發
8. 建築進入偵測
9. 結局 / 章節 gate 輪詢
10. `SettleRoster()`（名冊抽換，讓轉場同幀生效）
11. `sweep_`（SweepSystem：幀末延遲刪除）

**設計意圖**：輸入邊緣計時已移入 `InputHandler`，FSM 轉場觀察者已移入 `SceneRouter`，模擬階段已拆成 `ISystem`。`GameController` 維持為協調者，把所有輔助類別組裝起來——「一個輔助類別負責一項具體職責」的 SRP 示範。

EventBus 以**依賴注入**方式傳入建構子（`EventBus& bus`）：正式版傳單例，測試可傳區域匯流排。解構子負責 `EventBus::Clear()`，確保 lambda 捕捉的參考不會懸空。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class GameController` | 禁止拷貝；建構子注入 `World&`/`EventBus&`；解構子 `Clear` bus。 |
| `GameController(World& world, EventBus& bus)` | 安裝 EventBus 訂閱者（`WireDefaultSubscribers` 等）；初始化 ISystem 管線與 `SceneRouter`。 |
| `Update()` | 推進世界一幀；依序執行上述完整的幀順序。 |
| `HandleEndingMenu()` → `bool` | 結局選單；凍結時回傳 `true`。 |
| `HandlePauseMenu()` → `bool` | 暫停選單（含說明）；凍結時回傳 `true`。 |
| `HandleDialog()` → `bool` | 對話推進＋攤主購買；凍結時回傳 `true`。 |
| `HandleInventory()` → `bool` | Tab 背包；凍結時回傳 `true`。 |
| `DispatchInteract()` | E 鍵互動派發；讀輸入、跑 QuestHook 表。 |
| `world_` | 注入的 `World&`（非擁有）。 |
| `bus_` | 注入的 `EventBus&`（非擁有）。 |
| `frameColliders_` | 重用的暫存碰撞盒 vector（每幀 reuse，避免重新配置）。 |
| `sceneRouter_` | `SceneRouter` 成員；FSM 轉場觀察者。 |
| `input_` | `InputHandler` 成員；按住 E 自動推進計時。 |
| `pendingVendor_` | `Vendor*` 非擁有觀察指標；攤主購買的跨幀狀態（null 表示無選單）。 |
| `advanceSystems_` | `vector<unique_ptr<ISystem>>`：Survival/Movement/Collision/Spawn 管線。 |
| `sweep_` | `SweepSystem` 成員：幀末延遲刪除階段。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/controller/InputHandler.h`、`include/game/controller/SceneRouter.h`、`include/game/controller/SimSystem.h`（ISystem + 各具體 system）、`include/game/state/SemesterState.h`、`include/engine/math/Rect.h`/`Vec2.h`。
- **被誰使用（往內）**：`include/app/scenes/GameplayScene.h`（以 `GameController` 驅動遊戲幀）；大量測試直接建構 `GameController` 做整合測試（`test_rain_survival`、`test_scriptinput_*`、`test_ch1_spine_reachable`、`test_ch4_*`、`test_ending_menu` 等）。
- **繼承 / 實作 / 體現**：體現 [MVC 核心（arch-mvc）](../concepts/arch-mvc.md)。
- **每幀管線 / MVC 角色**：**MVC 的 Controller**；每幀管線的最外層協調者，執行 Survival → Movement → Collision → Spawn →（互動 / 結局判定）→ Sweep 的完整順序。

## OO 概念與設計重點

`GameController` 是 [MVC 架構（arch-mvc）](../concepts/arch-mvc.md) 的 Controller 核心：它收輸入、推進 Model（`World`），不接觸 View。每幀的各畫面凍結 handler（`HandleEndingMenu`/`HandlePauseMenu`/`HandleDialog`/`HandleInventory`）回傳 `bool` 作為「此幀已被此畫面持有」的訊號，讓主流程以早期回傳模式（guard clause）優雅處理，而非深層巢狀 if。

`ISystem` 管線採 [Strategy / Pipeline（pat-strategy）](../concepts/pat-strategy.md) 模式：各階段彼此解耦，`GameController` 以統一的 `Run(ctx, dt)` 介面執行，新增階段只需插入 vector，主流程不變。

EventBus 的**依賴注入**配合解構子的 `Clear()`，是確保 lambda 生命週期安全的關鍵——體現了 [RAII（oo-raii）](../concepts/oo-raii.md) 在 Observer 訂閱管理上的應用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/GameController.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameController.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [ISystem](../concepts/arch-isystem.md) · [Strategy](../concepts/pat-strategy.md)
