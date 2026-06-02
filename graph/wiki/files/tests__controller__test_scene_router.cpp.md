---
id: "file:tests/controller/test_scene_router.cpp"
type: test
path: tests/controller/test_scene_router.cpp
domain: tests
bucket: controller
loc: 332
classes: []
sources: ["tests/controller/test_scene_router.cpp"]
---
# `test_scene_router.cpp`

> **一句定位**：驗證 `SceneRouter` 的「雙 cursor 拆分」設計——`SettleRoster` 與 `SettleSideEffects` 各司其職、各自冪等，且轉場時副作用（玩家位移、消耗品清空、傘清除、抵達提示）正確套用。

## 職責

本檔包含 10 個 `TEST_CASE`，詳細釘住 `SceneRouter` 拆分前後的不變式。`SceneRouter` 將「換 roster」（`SettleRoster`，Update 末段）與「套用副作用」（`SettleSideEffects`，Update 前段）分成兩個半部，解決 roster 延遲一格可見的 bug。

**建構合約**：兩個 cursor（`LastRosterState`、`LastRosterRespawnState`）皆從傳入的初始狀態起算，`InterludeExitLatchMut() == false`。

**`SettleRoster` 行為**：FSM 未移動時為 no-op；FSM 移動後立即重生新章節 NPC（不再慢一格）；不會動到 `SettleSideEffects` 的 cursor（拆分確實生效，兩個 cursor 分開維護）。

**`SettleSideEffects` 進入 Interlude**：玩家位置傳送到 `kInterludeEntry`、清空消耗品（EnergyDrink 歸 0）、發出 `kInterludeArrivalHint` ShowMessage、重置 `InterludeExitLatch`；cursor 戳記後重複呼叫為冪等。

**`SettleSideEffects` 進入 Ch4**：清除 `HasUmbrella` 與 `kFlagHasTrueUmbrella`。

**進入 Ch2/Ch3 清除手持傘**：每章「傘又掉了」機制：進入 Ch2 清除 `HeldUmbrella::True` 與旗標；進入 Ch3 清除 `HasUmbrella` 與 `kFlagLibrarianUmbrella`。

**端到端流程**：同一 tick 先 `SettleRoster` 再 `SettleSideEffects`，驗證 SettleRoster 不發抵達提示（那是下一格 SettleSideEffects 的工作），兩半各自冪等不重複執行。

**`SettleRoster` 被略過的防禦性**：若 `SettleRoster` 未被呼叫，`SettleSideEffects` 的 respawn cursor 與 FSM 不一致時，會防禦性地呼叫 `RespawnChapterRoster` 保持 roster 一致。

**跨 Interlude 背包倖存者**：Ch1 → Interlude（清消耗品）→ Ch2（清傘），背包最終只剩 `kItemMoney` 與 `kItemForm` 兩列，沒有傘也沒有消耗品。

## 關鍵內容（類別 / 函式 / 資料）

- `SceneRouter`（被測型別）：`SettleRoster(World&)`、`SettleSideEffects(World&)`、`LastRosterState()`、`LastRosterRespawnState()`、`InterludeExitLatchMut()`。
- `HasNpcId(world, id)`：在 `Objects()` 中尋找 NPC 的 helper。
- `SubscribeToLatest(latest)`：訂閱 `ShowMessage` 並以 `ScopedSubscribe` 記錄最新文字，確保 handler 生命週期安全。
- `nccu::kInterludeEntry`：市集入口的固定座標。
- `nccu::kInterludeArrivalHint`：抵達市集的 HUD 提示文字。
- `nccu::BuildInventoryRows(*p)`：驗證背包列的輔助函式（`ItemCatalog`）。

## 相依與在架構中的位置
- **#include（往外）**：`SceneRouter.h`、`EventWiring.h`、`EventBus.h`、`World.h`、`Player.h`、`Flags.h`、`ItemCatalog.h`、`ChapterToast.h`、`InterludeExit.h`、`Vec2.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層；`SceneRouter` 坐落在 Update 的 roster 換班（末段）與副作用套用（前段）之間

## OO 概念與設計重點

本檔釘住了一個典型的「雙 cursor 冪等拆分」設計決策，解決了 Update 管線中 roster 可見性的 off-by-one-frame bug。`SubscribeToLatest` 使用 `ScopedSubscribe` 體現 [RAII](../concepts/oo-raii.md) 的測試安全性。端到端 case 涵蓋了整個 Ch1 → Interlude → Ch2 的背包倖存者合約，是遊戲劇情節奏的機制保證。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_scene_router.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_scene_router.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-mvc](../concepts/arch-mvc.md) · [RAII](../concepts/oo-raii.md)
