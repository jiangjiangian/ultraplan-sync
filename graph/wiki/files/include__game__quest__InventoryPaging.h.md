---
id: file:include/game/quest/InventoryPaging.h
type: header
path: include/game/quest/InventoryPaging.h
domain: game
bucket: quest
loc: 24
classes: []
sources: ["include/game/quest/InventoryPaging.h"]
---
# `InventoryPaging.h`

> **一句定位**：Tab 背包面板每頁顯示列數的唯一定義點（`kInventoryRowsPerPage = 6`），讓 game 層控制器讀取此常數而不必引入 ui 渲染標頭。

## 職責

`InventoryPaging.h` 是一個極度精簡的純常數標頭，僅宣告 `inline constexpr int kInventoryRowsPerPage = 6`，供背包面板的分頁計算使用。

此常數原本位於 `ui/InventoryView.h`，但 game 層的控制器（`InventoryScreen.cpp`、`GameController.cpp`）需要存取這個值來計算翻頁偏移（`±kInventoryRowsPerPage`）。如果保留在 `ui/` 標頭中，game 層引用它就會產生 game→ui 的反向相依，破壞架構紅線（相依方向必須是 app→game/ui→engine）。將其抽出到 `game/quest/` 中既讓 game 層可以安全引用，又保持 `ui/InventoryView.h` 仍能使用同一值（ui 消費 `kInventoryRowsPerPage` 用於 `InventoryPageOf` 推導顯示哪一頁）。

View 的行為語意：顯示的頁面永遠是包含游標的那一頁，因此游標永不會跑到畫面外；左右翻頁動作讓游標移動 ±`kInventoryRowsPerPage`。

## 關鍵內容（類別 / 函式 / 資料）

- `kInventoryRowsPerPage`（`inline constexpr int = 6`）：背包每頁固定顯示的列數，驅動 `InventoryPageOf` 計算與翻頁游標步進。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 #include）
- **被誰使用（往內）**：`include/ui/InventoryView.h`（`InventoryPageOf` 計算）、`src/game/controller/GameController.cpp`（翻頁輸入處理）、`src/game/controller/screens/InventoryScreen.cpp`（背包畫面翻頁邏輯）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純常數定義；game 層（Controller）讀取以處理翻頁輸入，View 層讀取以計算顯示哪一頁。

## OO 概念與設計重點

這個僅 24 行的極小標頭體現了**依賴反向原則（DIP）**在架構層面的應用：為了阻斷 game→ui 的非法相依，專門建立一個 game 層常數標頭，讓 game 層控制器與 ui 層 View 都引用同一個來源，而不是由 game 引用 ui 標頭。代碼注釋明確說明「兩者須同步，由 ui 端 static_assert 把關」，是小而實際的架構守衛模式。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/InventoryPaging.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/InventoryPaging.h) · [← 全檔索引](../files-index.md)
