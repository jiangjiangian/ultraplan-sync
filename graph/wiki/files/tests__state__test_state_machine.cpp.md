---
id: file:tests/state/test_state_machine.cpp
type: test
path: tests/state/test_state_machine.cpp
domain: tests
bucket: state
loc: 57
classes: []
sources: ["tests/state/test_state_machine.cpp"]
---
# `test_state_machine.cpp`

> **一句定位**：`SemesterStateMachine` 的基礎契約測試：初始狀態、`Transition` 後的狀態與顯示名稱，以及 `Update` 在任何狀態（含結局狀態下 `state_` 為 null）下都安全。

## 職責

此測試檔是 `SemesterStateMachine` 最基礎的單元測試，覆蓋四個 TEST_CASE：

1. **初始狀態**：新建的狀態機必須停在 `Chapter1_AddDrop`，`CurrentName()` 返回 `"第一章 加退選"`。

2. **Transition + CurrentName 更新**：切換到 `Interlude_Market` → `Current()` == 幕間，`CurrentName() == "幕間 市集"`；再切到 `Chapter4_Finals` → `CurrentName() == "第四章 期末"`。

3. **結局狀態名稱**：切換到 A/B/D/C 四個結局狀態後，`CurrentName()` 分別返回 `"結局 A"/"結局 B"/"結局 D"/"結局 C"`；`Ending_D` 與其他結局同級，是合法的終點狀態。

4. **`Update` 安全性**：在任何狀態（包含結局下 `state_` 可能為 nullptr）呼叫 `m.Update(0.016f)` 都不應崩潰；本 TEST_CASE 先在 Ch1 呼叫，再切到 `Ending_A` 後再呼叫。

## 關鍵內容（類別 / 函式 / 資料）

- `SemesterStateMachine::Current()`：返回當前 `SemesterState`。
- `SemesterStateMachine::CurrentName()`：返回當前狀態的顯示名稱字串。
- `SemesterStateMachine::Transition(SemesterState)`：切換狀態。
- `SemesterStateMachine::Update(float dt)`：每幀更新，結局狀態下 `state_` 為 null 時必須安全（null 檢查）。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterStateMachine.h`（唯一 include）。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層的學期狀態機基礎行為，對應 `GameController` 每幀呼叫的 `m.Update(dt)`。

## OO 概念與設計重點

`Update` 安全性測試是 [State 模式](../concepts/pat-state.md)的關鍵不變量：結局狀態下 `IChapterState` 指標為 null，若 `Update` 不先檢查就 dispatch 會引發空指標崩潰。此測試確保四結局狀態的「終止狀態」語義被正確實作（`state_ == nullptr` 時 `Update` 為 no-op）。`CurrentName()` 的完整覆蓋確保 UI 顯示名稱不因新增狀態而遺漏。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_state_machine.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_state_machine.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
