---
id: file:include/game/state/Chapter3SportsDay.h
type: header
path: include/game/state/Chapter3SportsDay.h
domain: game
bucket: state
loc: 27
classes: [Chapter3SportsDay]
sources: ["include/game/state/Chapter3SportsDay.h"]
---
# `Chapter3SportsDay.h`

> **一句定位**：State 模式的第三章具體狀態物件，僅回報自身 `Id()` 為 `Chapter3_SportsDay` 與顯示名稱「第三章 運動會」。

## 職責

`Chapter3SportsDay.h` 定義繼承自 `IChapterState` 的 `Chapter3SportsDay` 類別，是 `SemesterStateMachine` 在轉移到 `Chapter3_SportsDay` 狀態時實例化的具體狀態物件。

與前兩章相同的薄設計，只覆寫 `Id()`（回傳 `SemesterState::Chapter3_SportsDay`）和 `Name()`（回傳 `"第三章 運動會"`）。第三章的所有活動邏輯（操場跑圈、物物交換鏈、Ch3 漣漪與告白 Prof. trap 旗標）位於 `WorldSportsLap`、`QuestHookTable` 的 `TryAdvanceCh3Trade`/`TryApplyCh3Ripple` 鉤子、`ChapterSpawns` 與 `QuestIndicator` 的 Ch3 指示燈邏輯中。

## 關鍵內容（類別 / 函式 / 資料）

- `class Chapter3SportsDay : public IChapterState`：27 行薄包裝。
- `Id() const → SemesterState`：回傳 `SemesterState::Chapter3_SportsDay`。
- `Name() const → string_view`：回傳 `"第三章 運動會"`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`
- **被誰使用（往內）**：`src/game/state/SemesterStateMachine.cpp`
- **繼承 / 實作 / 體現**：繼承自 `IChapterState`
- **每幀管線 / MVC 角色**：Model 層 State 機的身分持有物；實際的 Ch3 邏輯（跑圈計時、交換鏈進度）由 World 子系統依 `Current() == Chapter3_SportsDay` 驅動。

## OO 概念與設計重點

[State 模式](../concepts/pat-state.md)標準葉節點。與 Ch1/Ch2 狀態物件共享相同的薄設計哲學，讓章節行為邏輯保留在各自功能模組（`WorldSportsLap`、`QuestHookTable`），不污染狀態物件本身。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/Chapter3SportsDay.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter3SportsDay.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
