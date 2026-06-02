---
id: file:include/game/state/SemesterState.h
type: header
path: include/game/state/SemesterState.h
domain: game
bucket: state
loc: 57
classes: [IChapterState]
sources: ["include/game/state/SemesterState.h"]
---
# `SemesterState.h`

> **一句定位**：學期流程的狀態列舉 `SemesterState` 與 State 模式抽象介面 `IChapterState` 的共同定義，是整個狀態機系統的核心基礎標頭。

## 職責

`SemesterState.h` 是所有學期狀態相關代碼的根基，定義了兩個核心型別：`SemesterState` 枚舉與 `IChapterState` 抽象介面。

`SemesterState` 列舉包含 9 個值：四個章節（`Chapter1_AddDrop`、`Chapter2_Midterms`、`Chapter3_SportsDay`、`Chapter4_Finals`）、一個幕間（`Interlude_Market`）與四種結局（`Ending_A`、`Ending_B`、`Ending_D`、`Ending_C`）。結局的枚舉順序刻意對應判定優先序 A→B→D→C（Ending_D 排在 Ending_C 之前），`Ending_D` 的注釋說明了「風雨同行」的語意（體諒助教但分數未達 A、亦未落入 B 的苦甜結局）。

`IChapterState` 是 State 模式的抽象狀態介面，定義了五個方法：純虛的 `Id()` 與 `Name()`，以及帶預設空實作的 `Enter(float)/Exit()/Update(float)`。`SemesterStateMachine` 持有 `unique_ptr<IChapterState>` 作為當前狀態，轉移時銷毀舊物件、建立新物件並依序呼叫 `Exit()`/`Enter()`。結局狀態（Ending_A/B/D/C）不另設 `IChapterState` 子類別，而以 `inEnding_` 旗標搭配 `ending_` 哨兵表示。

本標頭被極廣泛地引用（被 30+ 個標頭/源文件引用，涵蓋 World、Controller、Dialog、Quest、UI 等各層），是系統中引用次數最多的標頭之一。

## 關鍵內容（類別 / 函式 / 資料）

- `enum class SemesterState`：9 個值，結局順序對應判定優先序 A→B→D→C。
- `class IChapterState`：抽象基底，定義 `Id()/Name()`（純虛）與 `Enter()/Exit()/Update(dt)`（預設空實作）。虛解構子（`= default`）確保多型刪除正確。

## 相依與在架構中的位置

- **#include（往外）**：`<string_view>`（`Name()` 回傳型別）
- **被誰使用（往內）**：廣泛——被 `GameController.h`、`SceneRouter.h`、`DialogOpener.h`、`DialogRepository.h`、`NPC.h`、所有 chapter quest headers、所有 state headers（`SemesterStateMachine.h`、`EndingMenuModel.h`、`InterludeMarket.h` 等）、World.h、多個 ui 標頭，以及大量 src/ 與測試檔案
- **繼承 / 實作 / 體現**：被繼承者（`inherited_by`）：`Chapter1AddDrop`、`Chapter2Midterms`、`Chapter3SportsDay`、`Chapter4Finals`、`InterludeMarket`；realizes [State 模式](../concepts/pat-state.md)
- **每幀管線 / MVC 角色**：Model 層核心——`SemesterState` 是全系統的章節身分令牌，幾乎所有以章節為鍵的決策（NPC 生成、任務配置、結局判定、UI 顯示）都以此枚舉為鍵。`IChapterState` 是狀態機可替換的行為單元。

## OO 概念與設計重點

本檔是 [State 模式](../concepts/pat-state.md)的**抽象狀態（Abstract State）**角色：`IChapterState` 定義了所有具體狀態物件的共同介面，`SemesterStateMachine` 只依賴此抽象而不依賴任何具體章節類別，實現了**依賴倒置原則（DIP）**。`Enter()/Exit()/Update()` 的空預設實作（Template Method 的空掛勾）讓薄狀態物件不必覆寫不需要的方法。枚舉值順序的刻意設計（結局按優先序排列）是把設計意圖編碼進型別系統的好例子，儘管 C++ 枚舉本身不強制此順序，注釋使設計意圖明確。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/SemesterState.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterState.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
