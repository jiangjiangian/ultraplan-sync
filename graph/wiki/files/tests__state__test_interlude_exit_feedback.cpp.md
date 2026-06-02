---
id: file:tests/state/test_interlude_exit_feedback.cpp
type: test
path: tests/state/test_interlude_exit_feedback.cpp
domain: tests
bucket: state
loc: 119
classes: []
sources: ["tests/state/test_interlude_exit_feedback.cpp"]
---
# `test_interlude_exit_feedback.cpp`

> **一句定位**：驗證幕間出口的玩家回饋：`MaybeAnnounceInterludeExit` 首次跨入南側帶狀區時發佈一次提示（具 latch 冪等性），以及抵達幕間的提示能經 EventBus 到達 HUD 訂閱者。

## 職責

此測試檔針對「幕間出口回饋」的三個子系統進行驗證，皆不需要完整的 Input/Renderer 堆疊：

1. **`InInterludeExitZone` 幾何固定**：y=1910 在帶狀區內（第一幀觸發點）；`kInterludeEntry` 不在區內。與 `test_interlude_exit.cpp` 中的幾何測試形成跨檔的一致性確認。

2. **`MaybeAnnounceInterludeExit` latch 語義**：
   - 首次呼叫（玩家第一次進入帶狀區）→ 發佈 `kInterludeExitPrep` 訊息，返回 true，`latched` 設為 true。
   - 後續呼叫（玩家在邊界來回）→ 不再發佈，返回 false，訊息計數維持 1。
   
3. **Latch 生命週期**：玩家離開市集再回來時，`GameController` 重置 `latched = false`；測試模擬此重置，確認下次進入帶狀區再次觸發一次。

4. **抵達提示到達 HUD**：發佈 `ShowMessage` 事件後，`WireHudMessageSubscriber` 接線的 `World::HudMessage()` 確實包含 `kInterludeArrivalHint` 字串。

匿名 namespace 提供 `SubscribeToAll`（收集所有 `ShowMessage` 到 vector）、`Contains`、`Count` 輔助函式。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::MaybeAnnounceInterludeExit(EventBus, bool& latched)`：封裝 latch 生命週期，由 `GameController` 每幀在幕間中呼叫。
- `nccu::kInterludeExitPrep`：離開提示字串常數。
- `nccu::kInterludeArrivalHint`：抵達提示字串常數。
- `nccu::WireHudMessageSubscriber(EventBus, World&)`：接線 `ShowMessage` → `World::HudMessage()`。
- `TEST_CASE("InInterludeExitZone：固定南側帶狀區的判定")`。
- `TEST_CASE("MaybeAnnounceInterludeExit：首次發佈一次，之後具冪等性")`。
- `TEST_CASE("幕間造訪 latch 生命週期：進入時重置，跨入帶狀區時觸發")`。
- `TEST_CASE("抵達幕間的提示經完整路徑送達 HUD 訂閱者")`。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterToast.h`、`EventBus.h`、`EventWiring.h`（`WireHudMessageSubscriber`）、`InterludeExit.h`（`MaybeAnnounceInterludeExit`、`kInterludeExitPrep`、`kInterludeArrivalHint`）、`World.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Controller 層每幀在幕間中觸發的回饋機制，以及 Model（`World::HudMessage()`）接收 ShowMessage 的管線。

## OO 概念與設計重點

Latch 是「每次造訪只觸發一次」的狀態模式變體，由 `bool& latched` 外部狀態實現，`GameController` 在重置時手動清除。`SubscribeToAll` + `Count` 的設計允許精確計數同一字串出現了幾次，確保「不洗版保證」——每個 latch 週期確實只發一次。[Observer 模式](../concepts/pat-observer.md) 讓回饋訊息與 HUD View 解耦。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_interlude_exit_feedback.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_interlude_exit_feedback.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
