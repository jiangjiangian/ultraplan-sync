---
id: file:include/game/state/Chapter2Midterms.h
type: header
path: include/game/state/Chapter2Midterms.h
domain: game
bucket: state
loc: 27
classes: [Chapter2Midterms]
sources: ["include/game/state/Chapter2Midterms.h"]
---
# `Chapter2Midterms.h`

> **一句定位**：State 模式的第二章具體狀態物件，僅回報自身 `Id()` 為 `Chapter2_Midterms` 與顯示名稱「第二章 期中考」。

## 職責

`Chapter2Midterms.h` 定義繼承自 `IChapterState` 的 `Chapter2Midterms` 類別，是 `SemesterStateMachine` 在轉移到 `Chapter2_Midterms` 狀態時實例化的具體狀態物件（State 模式的葉節點）。

與 `Chapter1AddDrop` 相同的薄設計：只覆寫 `Id()`（回傳 `SemesterState::Chapter2_Midterms`）與 `Name()`（回傳 `"第二章 期中考"`），`Enter()`/`Exit()`/`Update()` 使用空預設實作。第二章的所有活動邏輯（圖書館管理員→學霸主線、三頁筆記生成與 Ch2 漣漪）位於 `QuestHookTable`、`ChapterSpawns`、`ChapterQuestItems` 與 `World` 的 `MaybeSpawnChapter2Notes` 中。

## 關鍵內容（類別 / 函式 / 資料）

- `class Chapter2Midterms : public IChapterState`：27 行薄包裝。
- `Id() const → SemesterState`：回傳 `SemesterState::Chapter2_Midterms`。
- `Name() const → string_view`：回傳 `"第二章 期中考"`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`
- **被誰使用（往內）**：`src/game/state/SemesterStateMachine.cpp`
- **繼承 / 實作 / 體現**：繼承自 `IChapterState`
- **每幀管線 / MVC 角色**：Model 層 State 機的身分持有物；實際的 Ch2 邏輯由 World/quest 子系統依 `Current() == Chapter2_Midterms` 驅動。

## OO 概念與設計重點

[State 模式](../concepts/pat-state.md)標準葉節點。薄狀態物件讓各章節的行為邏輯保持在其所屬的模組中（遵循 SRP），`SemesterStateMachine` 只需持有抽象 `IChapterState*` 即可判斷當前章節，實現多型替換（Liskov Substitution Principle）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/Chapter2Midterms.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter2Midterms.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
