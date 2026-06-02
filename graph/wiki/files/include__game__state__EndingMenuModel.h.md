---
id: file:include/game/state/EndingMenuModel.h
type: header
path: include/game/state/EndingMenuModel.h
domain: game
bucket: state
loc: 54
classes: []
sources: ["include/game/state/EndingMenuModel.h"]
---
# `EndingMenuModel.h`

> **一句定位**：結局畫面選單的 game 層模型——結局狀態述詞、三選項列舉 `EndingMenuChoice` 及其索引/標籤對映，斷開 game→ui 的反向相依。

## 職責

`EndingMenuModel.h` 把結局畫面選單所需的 game 層邏輯（「現在是結局嗎」、「索引 N 對應什麼選擇」、「選擇的顯示標籤」）集中宣告，使 game 層的場景、控制器、狀態機觀察者能讀取這些信息而不必引入 `ui/EndingView.h` 渲染標頭，符合架構相依紅線（game 不應引入 ui）。

`IsEndingState(SemesterState)` 是純狀態述詞，判斷是否為 Ending_A/B/D/C 之一，用於結局場景的「凍結守衛」——一旦進入結局，世界場景暫停而結局選單掌控當前幀。

`EndingMenuChoice` 枚舉（`BackToTitle / RestartGame / Quit`）是三個選項的語意唯一來源，對應游標索引 0/1/2，唯有 `Quit` 會關閉視窗。`EndingMenuChoiceAt(int index)` 提供游標索引→選擇的安全對映（越界時夾鉗）。`EndingMenuLabel(EndingMenuChoice)` 提供繁中顯示標籤，View 只需渲染被選取的列。

兩種路由（`BackToTitle`/`RestartGame`）透過 `World::PendingAppAction` 實現，`Quit` 觸發視窗關閉；View 只繪製，不做路由判斷。

## 關鍵內容（類別 / 函式 / 資料）

- `IsEndingState(SemesterState s) noexcept → bool`：判斷 s 是否為四種結局之一。
- `enum class EndingMenuChoice`：`BackToTitle`（0）、`RestartGame`（1）、`Quit`（2）。
- `EndingMenuChoiceAt(int index) noexcept → EndingMenuChoice`：索引→選擇，越界時夾鉗。
- `EndingMenuLabel(EndingMenuChoice c) noexcept → string_view`：選擇→繁中標籤。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`（`SemesterState` 型別、`IsEndingState` 參數）
- **被誰使用（往內）**：`include/ui/EndingView.h`（View 渲染）、`src/game/controller/GameController.cpp`（結局選單操作）、`src/game/controller/screens/EndingScreen.cpp`（結局畫面控制）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層——`IsEndingState` 作為守衛，`EndingMenuChoiceAt`/`EndingMenuLabel` 供 Controller 處理輸入與 View 渲染使用；體現 [MVC](../concepts/arch-mvc.md) 分層：Model 提供語意，View 只呈現。

## OO 概念與設計重點

本檔是**架構依賴方向管理**的典型案例：透過在 game 層定義選單語意（枚舉、述詞、標籤），使 game 層控制器（`EndingScreen`）可以處理輸入邏輯而不引入任何 ui 渲染標頭，完全符合 [MVC](../concepts/arch-mvc.md) 分層。`EndingMenuChoiceAt` 的越界夾鉗（而非 assert 或返回「無效」值）是防禦性編程的體現，確保游標永不選到未定義狀態。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/EndingMenuModel.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/EndingMenuModel.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
