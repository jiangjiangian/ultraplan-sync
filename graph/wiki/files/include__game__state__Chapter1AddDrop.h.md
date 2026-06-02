---
id: file:include/game/state/Chapter1AddDrop.h
type: header
path: include/game/state/Chapter1AddDrop.h
domain: game
bucket: state
loc: 28
classes: [Chapter1AddDrop]
sources: ["include/game/state/Chapter1AddDrop.h"]
---
# `Chapter1AddDrop.h`

> **一句定位**：State 模式的第一章具體狀態物件，僅回報自身 `Id()` 為 `Chapter1_AddDrop` 與顯示名稱「第一章 加退選」，章節邏輯分散於 World 與 quest 層。

## 職責

`Chapter1AddDrop.h` 定義繼承自 `IChapterState` 的 `Chapter1AddDrop` 類別，是 `SemesterStateMachine` 在轉移到 `Chapter1_AddDrop` 狀態時實例化的具體狀態物件（State 模式的葉節點）。

類別只覆寫兩個純虛函式：`Id()` 回傳 `SemesterState::Chapter1_AddDrop`，`Name()` 回傳 `"第一章 加退選"`。`Enter()`、`Exit()`、`Update()` 全部使用基底類別的空預設實作，因為第一章的所有活動邏輯（NPC 生成、任務配置、碰撞、對話）分散在 World 的 `RespawnChapterRoster`、`QuestHookTable` 與各 quest 標頭中，而非集中於狀態物件內。

這個設計選擇（薄狀態物件）的優點是每個章節狀態物件只做「身分識別」，讓複雜的章節邏輯按照職責分散到 game 層各個子模組，而 `SemesterStateMachine` 只用 `state_->Id()` 判斷當前章節。

## 關鍵內容（類別 / 函式 / 資料）

- `class Chapter1AddDrop : public IChapterState`：28 行的薄包裝，只覆寫兩個方法。
- `Id() const → SemesterState`：回傳 `SemesterState::Chapter1_AddDrop`。
- `Name() const → string_view`：回傳 `"第一章 加退選"`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`（`IChapterState` 基底類別與 `SemesterState` 列舉）
- **被誰使用（往內）**：`src/game/state/SemesterStateMachine.cpp`（在 `Transition(Chapter1_AddDrop)` 時 `new Chapter1AddDrop`）
- **繼承 / 實作 / 體現**：繼承自 `IChapterState`（定義於 `SemesterState.h`）
- **每幀管線 / MVC 角色**：Model 層 State 機的身分持有物；`SemesterStateMachine` 透過 `state_->Id()` 感知當前章節，實際管線邏輯（Spawn / Quest / 結局判定）由其他子系統依此 id 驅動。

## OO 概念與設計重點

本檔是 [State 模式](../concepts/pat-state.md)的標準具體狀態（Concrete State）：`SemesterStateMachine` 作為 Context 持有 `IChapterState*`，轉移時銷毀舊狀態物件並建立新的（此為 `Chapter1AddDrop`），不需改 Context 類別。薄狀態物件的設計使章節的「是什麼」（身分）與「做什麼」（邏輯）分開，是**單一責任原則**在狀態層面的體現。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/Chapter1AddDrop.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter1AddDrop.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
