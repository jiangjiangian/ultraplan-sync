---
id: file:tests/quest/test_chapter_gate.cpp
type: test
path: tests/quest/test_chapter_gate.cpp
domain: tests
bucket: quest
loc: 55
classes: []
sources: ["tests/quest/test_chapter_gate.cpp"]
---
# `test_chapter_gate.cpp`

> **一句定位**：驗證章節閘門的事件接線：Ch1 領到 `TrueUmbrella` 才推進到幕間市集，非 True 傘或非 Ch1 狀態不誤觸，`EnteredBuilding` 只更新名稱不跳章。

## 職責

此測試檔為 `nccu::WireStateTransitionSubscribers` 接線後的 `EventBus` → `SemesterStateMachine` 管線提供四個邊界測試，確保章節閘門的觸發條件精確且不漏。

四個 TEST_CASE：
1. Ch1 中 `UmbrellaClaimed("TrueUmbrella")` → 轉到 `Interlude_Market`。
2. Ch1 中 `UmbrellaClaimed("FragileUmbrella")` → 停留 Ch1（非 True 傘不推進）。
3. 已在 `Interlude_Market` 時收到 `UmbrellaClaimed("TrueUmbrella")` → 停留幕間（閘門僅在 Ch1 觸發）。
4. `EnteredBuilding("正門")` → `name == "正門"` 但 `m.Current()` 仍在 Ch1（建築進入只更新名稱，曾誤觸跳章的回歸測試）。

每個 TEST_CASE 前後都呼叫 `EventBus::Instance().Clear()` 確保訂閱者隔離。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name)`：設定 `UmbrellaClaimed` 與 `EnteredBuilding` 訂閱者。
- `TEST_CASE("章節閘門：在 Ch1 領取 TrueUmbrella -> 幕間市集")`。
- `TEST_CASE("章節閘門：非 True 的雨傘不會推進 Ch1")`。
- `TEST_CASE("章節閘門：非 Ch1 時領取 TrueUmbrella -> 不轉場")`。
- `TEST_CASE("章節閘門：進入建築不再連帶跳章")`：回歸測試，固定曾修正的錯誤行為。

## 相依與在架構中的位置

- **#include（往外）**：`EventWiring.h`（`WireStateTransitionSubscribers`）、`SemesterStateMachine.h`、`EventBus.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Controller 層的事件接線，對應每幀管線中 `EventBus` 接收 `UmbrellaClaimed` 後觸發的狀態轉場。

## OO 概念與設計重點

此測試體現 [Observer 模式](../concepts/pat-observer.md)的隔離驗證：`WireStateTransitionSubscribers` 把事件訂閱登記到全局 `EventBus`，測試直接發布事件而不需要建構 `GameController`。第四個 TEST_CASE 是典型的「回歸網」——修正曾發生的 bug（建築進入誤跳章）後，用測試鎖住正確行為防止再犯。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter_gate.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter_gate.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
