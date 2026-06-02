---
id: file:tests/quest/test_chapter_spine.cpp
type: test
path: tests/quest/test_chapter_spine.cpp
domain: tests
bucket: quest
loc: 127
classes: []
sources: ["tests/quest/test_chapter_spine.cpp"]
---
# `test_chapter_spine.cpp`

> **一句定位**：驗證完整章節主幹的可重入走訪序列（Ch1→市→Ch2→市→Ch3→市→Ch4），含 `Flag_LeaveInterlude` 的消耗、`InterludeReturnTo` 的更新、轉場時關閉對話，以及沒有旗標時任一章節都不誤轉。

## 職責

此測試檔是整個章節推進邏輯的「骨幹整合測試」，以 `CheckChapterGates` 為核心驅動點，驗證七段主線轉場的每個節點都正確。

第一個 TEST_CASE 走完整序列：
- Ch1 → 幕間：`UmbrellaClaimed("TrueUmbrella")` 觸發，`InterludeReturnTo` 設為 Ch2。
- 幕間 → Ch2：設 `kFlagLeaveInterlude`，`CheckChapterGates` 消耗旗標，旗標消耗後不再存在。
- Ch2 → 幕間：設 `kFlagCh2Cleared`，`returnTo` 更新為 Ch3。
- 幕間 → Ch3：消耗旗標，`returnTo` 更新為 Ch4。
- Ch3 → 幕間：設 `kFlagCh3Cleared`，`returnTo` 更新為 Ch4。
- 幕間 → Ch4：消耗旗標；健全性確認 Ch4 沒有同層 if 轉場。

第二個 TEST_CASE 驗證「重新進入市集不會立刻退出」：旗標在第一次離開時被消耗，重新進入後沒有新旗標就不應退出。

第三個 TEST_CASE 驗證「轉場時閘門會關閉仍開啟中的對話」：以 `d.Open({"queued line"})` 模擬對話中，設 `kFlagCh2Cleared` 後 `CheckChapterGates` 不僅轉場且 `d.Active()` 變 false。

第四個 TEST_CASE 驗證「沒有任何旗標 → 任一章節都不轉場」：Ch1/Ch2/幕間在無旗標下分別呼叫 `CheckChapterGates`，狀態機不動。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::CheckChapterGates(EventBus::Instance(), p, m, d)`：被測的章節閘門輪詢函式。
- `nccu::WireStateTransitionSubscribers`：在第一個 TEST_CASE 中設定幕間入口訂閱。
- `m.InterludeReturnTo()`：確認幕間離開後的目的章節是否正確更新。
- `TEST_CASE("章節主幹：Ch1 -> 市 -> Ch2 -> 市 -> Ch3 -> 市 -> Ch4")`：完整骨幹走訪。
- `TEST_CASE("章節主幹：重新進入市集不會立刻退出（旗標已消耗）")`。
- `TEST_CASE("章節主幹：轉場時閘門會關閉仍開啟中的對話")`。
- `TEST_CASE("章節主幹：沒有任何旗標 -> 任一章節都不轉場")`。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`ChapterGate.h`（`CheckChapterGates`）、`EndingGate.h`、`EventWiring.h`、`SemesterStateMachine.h`、`EventBus.h`、`DialogState.h`、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試每幀管線中 `CheckChapterGates` 的行為（位於 Sweep 之前的結局/章節判定呼叫點）。

## OO 概念與設計重點

`returnTo` 存在狀態機上（而非 `InterludeMarket` 物件）是一個設計上的關鍵決策：幕間可以進入多次（三次），每次 `returnTo` 指向不同的下一章，若存在 `InterludeMarket` 物件則每次重建都需傳參。`kFlagLeaveInterlude` 的「消耗即消滅」設計確保旗標不會跨幀殘留。[State 模式](../concepts/pat-state.md)的 `SemesterStateMachine` 在此驗證其轉場不變量。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter_spine.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter_spine.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md)
