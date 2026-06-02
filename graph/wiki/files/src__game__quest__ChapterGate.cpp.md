---
id: file:src/game/quest/ChapterGate.cpp
type: source
path: src/game/quest/ChapterGate.cpp
domain: game
bucket: quest
loc: 52
classes: []
sources: ["src/game/quest/ChapterGate.cpp"]
---
# `ChapterGate.cpp`

> **一句定位**：章節通關閘門——每非對話幀輪詢，檢測通關旗標並觸發 FSM 轉場到插曲市集或下一章。

## 職責

`CheckChapterGates(EventBus&, Player&, SemesterStateMachine&, DialogState&)` 是在 `GameController` 每幀管線中輪詢的章節推進器，負責把「通關旗標已滿足」的訊號轉換為 FSM 狀態轉場。

**第二章 → 插曲段**：若當前章節為 Ch2 且 `kFlagCh2Cleared` 已設，呼叫 `semester.SetInterludeReturnTo(Chapter3_SportsDay)` + `semester.Transition(Interlude_Market)` + `PublishChapterTransitionToast(bus, Interlude_Market)` + `dialog.Close()`。Toast 在 `Transition()` 之後發布，確保訂閱者查詢時讀到已切換後的狀態。

**第三章 → 插曲段**：同樣邏輯，以 `kFlagCh3Cleared` 為閘，`returnTo` 設為 `Chapter4_Finals`。

**插曲段 → 返回章節**：若當前為插曲段且 `kFlagLeaveInterlude` 已設，先清除旗標（避免重入），再取 `semester.InterludeReturnTo()` 呼叫 `Transition` + `PublishChapterTransitionToast` + `dialog.Close()`。第一章→插曲段由 `UmbrellaClaimed` 接線的 EventWiring 驅動（非此函式管轄），第二/第三章清關走此函式。

## 關鍵內容（類別 / 函式 / 資料）

- `CheckChapterGates(EventBus&, Player&, SemesterStateMachine&, DialogState&)`：唯一入口，三個 if 分支。
- `semester.SetInterludeReturnTo(SemesterState)` / `semester.InterludeReturnTo()`：插曲段返回目標的讀寫。
- `semester.Transition(SemesterState)`：FSM 狀態轉場。
- `PublishChapterTransitionToast(bus, state)`：發布章節過場提示（引自 `ChapterToast.h`）。
- `player.ClearFlag(kFlagLeaveInterlude)`：先消費旗標再轉場，防止重入。
- `kFlagCh2Cleared` / `kFlagCh3Cleared` / `kFlagLeaveInterlude`（引自 `Flags.h`）：通關旗標。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterGate.h`、`Flags.h`（通關旗標）、`ChapterToast.h`（`PublishChapterTransitionToast`）、`EventBus.h`（Toast 發布）、`Player.h`（旗標查詢/清除）、`SemesterStateMachine.h`（FSM 轉場）、`SemesterState.h`（狀態列舉）、`DialogState.h`（`dialog.Close()`）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 每幀管線的結局/通關判定階段呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：每幀管線的「結局/通關判定」階段；在 Movement / Collision 後、Sweep 前輪詢。

## OO 概念與設計重點

本檔是 [State 模式](../concepts/pat-state.md) 的外部觸發器：`SemesterStateMachine` 的轉場不由實體或對白系統直接呼叫，而由統一的 `CheckChapterGates` 閘門管理，確保所有章節轉場都經過同一道旗標前置條件檢查。「先消費 `kFlagLeaveInterlude` 再轉場」的順序防止重入是旗標驅動系統中常見且重要的不變式維護。[Observer](../concepts/pat-observer.md) 體現於 `PublishChapterTransitionToast`——HUD 訂閱者顯示過場提示，`ChapterGate.cpp` 不感知 UI。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/ChapterGate.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ChapterGate.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md)
