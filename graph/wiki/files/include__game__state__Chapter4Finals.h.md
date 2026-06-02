---
id: file:include/game/state/Chapter4Finals.h
type: header
path: include/game/state/Chapter4Finals.h
domain: game
bucket: state
loc: 27
classes: [Chapter4Finals]
sources: ["include/game/state/Chapter4Finals.h"]
---
# `Chapter4Finals.h`

> **一句定位**：State 模式的第四章具體狀態物件，僅回報自身 `Id()` 為 `Chapter4_Finals` 與顯示名稱「第四章 期末」；結局判定由 `EndingGate` 輪詢旗標驅動，不在此處。

## 職責

`Chapter4Finals.h` 定義繼承自 `IChapterState` 的 `Chapter4Finals` 類別，是第四章的薄狀態物件。文件注釋特別強調：結局的判定「不在此處」，而由 `EndingGate::CheckEndingGates()` 在本章每個非對話幀輪詢旗標與業力值後決定，觸發 `SemesterStateMachine::Transition()` 進入對應結局。

`Chapter4Finals` 只覆寫 `Id()`（回傳 `SemesterState::Chapter4_Finals`）和 `Name()`（回傳 `"第四章 期末"`），`Enter()`/`Exit()`/`Update()` 使用空預設實作。第四章的活動邏輯（助教終局選擇、傘碰觸觸發、Ch4 漣漪）分散在 `QuestHookTable`（`TryApplyCh4Ripple`）、`EndingGate`、`ChapterSpawns`（Ch4 名冊）中。

## 關鍵內容（類別 / 函式 / 資料）

- `class Chapter4Finals : public IChapterState`：27 行薄包裝。
- `Id() const → SemesterState`：回傳 `SemesterState::Chapter4_Finals`。
- `Name() const → string_view`：回傳 `"第四章 期末"`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`
- **被誰使用（往內）**：`src/game/state/SemesterStateMachine.cpp`
- **繼承 / 實作 / 體現**：繼承自 `IChapterState`
- **每幀管線 / MVC 角色**：Model 層 State 機的身分持有物；結局判定由 `EndingGate::CheckEndingGates()` 在 Controller 的每幀管線中（Sweep 前）呼叫，以 `Current() == Chapter4_Finals` 為前提條件。

## OO 概念與設計重點

[State 模式](../concepts/pat-state.md)標準葉節點。值得注意的是 Ch4 狀態物件特意不做任何結局邏輯，因為結局是「閘門驅動」（`EndingGate` 輪詢），而非「狀態驅動」（`Update()` 鉤子）——這是一個刻意的架構選擇，使結局判定邏輯集中在 `EndingGate.cpp` 而不散落到狀態物件中。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/Chapter4Finals.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter4Finals.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
