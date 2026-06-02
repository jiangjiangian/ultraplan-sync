---
id: file:src/game/state/SemesterStateMachine.cpp
type: source
path: src/game/state/SemesterStateMachine.cpp
domain: game
bucket: state
loc: 99
classes: []
sources: ["src/game/state/SemesterStateMachine.cpp"]
---
# `SemesterStateMachine.cpp`

> **一句定位**：學期狀態機的建構與轉移實作，維護「具體章節 / 幕間以 `unique_ptr<IChapterState>` 表示、四種結局以 `inEnding_` 哨兵表示」這一核心不變式。

## 職責

此檔屬於 game / state 層，是 `SemesterStateMachine` 類別的成員函式實作。

建構式 `SemesterStateMachine()` 以 `make_unique<Chapter1AddDrop>()` 初始化 `state_` 並呼叫 `state_->Enter()`，使遊戲一開始就進入第一章狀態。

`Transition(SemesterState next)` 是最核心的方法：先呼叫舊狀態的 `state_->Exit()`，再依 `next` 以 switch 建立對應的具體 `IChapterState`（`Chapter1AddDrop`、`InterludeMarket`、`Chapter2Midterms`、`Chapter3SportsDay`、`Chapter4Finals`），或對四個 `Ending_*` 目標執行 `state_.reset()`、設下 `ending_` 哨兵並將 `inEnding_` 旗標改為 true（不建立任何 `IChapterState` 物件）。新建的狀態最後呼叫 `Enter()`。

`Current()` 與 `CurrentName()` 先檢查 `inEnding_`：若為 true 則回傳哨兵值或對應的結局中文名稱（「結局 A/B/D/C」），否則委派給 `state_->Id()` / `state_->Name()`。`Update(dt)` 同樣以 `state_` 非 null 為守衛，只在非結局狀態下推進章節更新。`SetInterludeReturnTo` / `InterludeReturnTo` 管理插曲段返回目標（用於 `CheckEndingGates` 與生成邏輯）。

## 關鍵內容（類別 / 函式 / 資料）

- `SemesterStateMachine()` — 建構式，初始化 `Chapter1AddDrop` 狀態並呼叫 `Enter()`。
- `Current() const noexcept` — 若在結局中回傳 `ending_` 哨兵，否則委派 `state_->Id()`。
- `CurrentName() const` — 若在結局中回傳中文結局名稱（`std::string_view`），否則委派 `state_->Name()`。
- `Transition(SemesterState)` — 舊狀態 Exit → 新狀態建立 → Enter；結局目標則 `state_.reset()` + 哨兵，不建立具體狀態類別。
- `SetInterludeReturnTo(SemesterState)` / `InterludeReturnTo()` — 插曲段返回章節的存取器。
- `Update(float dt)` — 委派 `state_->Update(dt)`，`state_` 為 null（結局中）時跳過。
- `state_` — `unique_ptr<IChapterState>`，五章及插曲共用；結局中為 null。
- `inEnding_` / `ending_` — 結局哨兵，使 `Current()` 無需為結局建立子類別。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterStateMachine.h`（類別宣告）、`Chapter1AddDrop.h` / `Chapter2Midterms.h` / `Chapter3SportsDay.h` / `Chapter4Finals.h` / `InterludeMarket.h`（五個具體 `IChapterState` 子類別）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `World` 組合擁有（`semester_` 成員），並由 `GameController`、`EndingGate`、各章節 Quest 函式等多處呼叫 `Current()` / `Transition()`。
- **繼承 / 實作 / 體現**：`SemesterStateMachine` 持有 `unique_ptr<IChapterState>`，是 [State](../concepts/pat-state.md) 模式的 Context 角色。
- **每幀管線 / MVC 角色**：Model 層（`game/state`）；`Update(dt)` 在每幀由 `World::Update` 推進，`Transition` 由 Controller 觸發，`Current()` 被 View 讀取（唯讀）。

## OO 概念與設計重點

此檔是 [State](../concepts/pat-state.md) 模式的標準 Context 實作：`Transition` 動態替換 `state_` 指標，使外部呼叫者（Controller / Quest 函式）只需呼叫 `Transition(next)` 而無需知道具體類別。結局不另建類別而以哨兵表示是刻意的簡化：結局不需要 `Enter/Exit/Update` 的生命週期方法，用 `state_.reset() + inEnding_` 哨兵足夠，且完全避免空物件或 Null Object 帶來的複雜度。`unique_ptr` 管理具體狀態物件生命週期，體現 [RAII](../concepts/oo-raii.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/state/SemesterStateMachine.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/state/SemesterStateMachine.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [RAII](../concepts/oo-raii.md)
