---
id: file:include/game/state/SemesterStateMachine.h
type: header
path: include/game/state/SemesterStateMachine.h
domain: game
bucket: state
loc: 67
classes: [SemesterStateMachine]
sources: ["include/game/state/SemesterStateMachine.h"]
---
# `SemesterStateMachine.h`

> **一句定位**：State 模式的 Context（上下文）——持有並管理 `IChapterState` 物件，驅動學期在四章節、幕間市集與四種結局之間的狀態轉移。

## 職責

`SemesterStateMachine.h` 宣告 `SemesterStateMachine` 類別，是 GoF State 模式中 Context 的角色。類別持有 `unique_ptr<IChapterState> state_` 表示當前狀態，轉移時銷毀舊物件、建立新物件並依序呼叫 `Exit()`/`Enter()`。

關鍵設計決策：結局狀態（Ending_A/B/D/C）不另設 `IChapterState` 子類別，而以 `bool inEnding_` 旗標搭配 `SemesterState ending_` 哨兵表示（此時 `state_` 為 `nullptr`）。這避免為四個幾乎沒有行為的結局狀態各自建立一個類別。

幕間返回目標 `interludeReturnTo_` 儲存在狀態機而非 `InterludeMarket` 物件上，設計注釋明確說明原因：每次 `Transition()` 都重建 `IChapterState`，狀態物件無法跨「章節→幕間→章節」往返保存資料，因此此跨轉移的目標必須存在更長命的 Context 中。

建構時立即進入 `Chapter1_AddDrop` 並觸發 `Enter()`。類別不可複製（deleted copy constructor 與 assignment），因為狀態機代表唯一的學期進度。

`Update(dt)` 委派給 `state_->Update(dt)`（結局期間 state_ 為 nullptr，此呼叫會跳過）。

## 關鍵內容（類別 / 函式 / 資料）

- `SemesterStateMachine()`：建構時進入初始狀態 `Chapter1_AddDrop`，觸發 `Enter()`。
- `Current() noexcept → SemesterState`：取當前狀態 id（結局期間回傳 `ending_`）。
- `CurrentName() → string_view`：取當前狀態繁中顯示名稱。
- `Transition(SemesterState next)`：章節/幕間建立對應 `IChapterState` 物件；結局改設 `inEnding_=true`, `ending_=next`。
- `Update(float dt)`：轉交當前 `state_->Update(dt)`。
- `SetInterludeReturnTo(SemesterState next) noexcept`：設定幕間後返回的章節。
- `InterludeReturnTo() noexcept → SemesterState`：讀取幕間返回目標（預設第二章）。
- `state_`（`unique_ptr<IChapterState>`）、`ending_`、`inEnding_`、`interludeReturnTo_`（私有成員）。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`（`IChapterState`、`SemesterState`）；標準庫 `<memory>`、`<string_view>`
- **被誰使用（往內）**：`include/game/controller/EventWiring.h`、`include/game/world/World.h`、`src/engine/platform/Harness.cpp`、`src/game/controller/screens/DialogScreen.cpp`、`src/game/quest/ChapterGate.cpp`、`src/game/state/EndingGate.cpp`、`src/game/state/SemesterStateMachine.cpp`（實作體）；大量測試（`test_ch1_quest.cpp`、`test_chapter_gate.cpp`、`test_chapter_spine.cpp`、`test_state_machine.cpp`、`test_ending_gate.cpp`、`test_interlude_exit.cpp` 等）
- **繼承 / 實作 / 體現**：realizes [State 模式](../concepts/pat-state.md)
- **每幀管線 / MVC 角色**：Model 層的核心狀態容器——`World` 持有 `SemesterStateMachine` 實例，Controller（GameController、ChapterGate、EndingGate）透過 `World::Semester()` 存取並呼叫 `Transition()`；`Update()` 在每幀管線中由 World 轉發。

## OO 概念與設計重點

本類別是 [State 模式](../concepts/pat-state.md)的 Context，持有 `IChapterState*` 並在轉移時替換物件，實現可替換的行為。`unique_ptr` 確保舊狀態物件在轉移時被正確銷毀（[RAII](../concepts/oo-raii.md)），不需手動 delete。「結局不設具體 IChapterState 子類別」的設計是刻意的工程簡化：避免四個無行為的薄類別，改以 `inEnding_` 旗標直接記錄，是實用主義對模式純粹性的平衡。`interludeReturnTo_` 儲存在 Context 而非 State 物件的設計精確體現了「橫跨多個狀態轉移的資料屬於 Context」的 State 模式最佳實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/SemesterStateMachine.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterStateMachine.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [RAII](../concepts/oo-raii.md)
