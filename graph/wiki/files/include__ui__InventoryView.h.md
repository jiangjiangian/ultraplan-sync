---
id: file:include/ui/InventoryView.h
type: header
path: include/ui/InventoryView.h
domain: ui
bucket: 
loc: 57
classes: []
sources: ["include/ui/InventoryView.h"]
---
# `InventoryView.h`

> **一句定位**：Tab 物品欄疊層的純渲染介面——`DrawInventory` 只讀 `InventoryRow` DTO 向量與游標索引，不碰 World，並提供可單元測試的分頁計算函式。

## 職責

本標頭定義物品欄渲染層的三個公開函式，遵守 MVC 純度：`DrawInventory` 只讀取傳入的 `InventoryRow` DTO 向量，不持有狀態，不存取 `World` 或 `Player`。DTO 由 game/controller 層的 `BuildInventoryRows` 從 World/Player 建出，再傳入此函式渲染，MVC 邊界明確。

兩個純計算函式 `InventoryPageCount` 與 `InventoryPageOf` 使分頁邏輯可以獨立於渲染器做單元測試：`InventoryPageCount(rowCount)` 以 `kInventoryRowsPerPage`（定義於 `game/quest/InventoryPaging.h`）計算總頁數（空背包為 1）；`InventoryPageOf(cursor, rowCount)` 先把游標夾限進 `[0, rowCount)` 再計算所在頁，確保越界游標也能得出有效頁碼。

`DrawInventory` 的分頁邏輯：列數超過 `kInventoryRowsPerPage` 時，只顯示游標所在頁（選取列永遠可見）並附「第 N／M 頁」指示。效果文字以 `dialog::WrapToCells` 折行，絕不溢出邊框。所有繪製經注入的 `IRenderer`，不直接呼叫 raylib，具決定性且可由無頭流程逐位元驗證。

## 關鍵內容（類別 / 函式 / 資料）

- `InventoryPageCount(rowCount) -> int`（`[[nodiscard]]`）：計算 `rowCount` 列的背包共有幾頁，恆 ≥1；純整數運算，可無頭單元測試。
- `InventoryPageOf(cursor, rowCount) -> int`（`[[nodiscard]]`）：計算游標所在頁（0 起算），游標先夾限 `[0, rowCount)` 確保越界游標不越界索引。
- `DrawInventory(r, rows, cursor, screenW, screenH)`：繪製 Tab 物品欄疊層——置中面板 + 金幣行 + 消耗品（含數量與描述）+ 攜帶傘 + 任務紙張 + 游標 + 說明面板；分頁顯示游標所在頁並附頁碼指示；空的 `rows` 畫「（空）」一行；效果文字折行不溢出。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/InventoryPaging.h`（`kInventoryRowsPerPage` 每頁列數常數）；`game/quest/ItemCatalog.h`（`InventoryRow` DTO 型別）；`<vector>`。
- **被誰使用（往內）**：`src/ui/InventoryView.cpp`（`DrawInventory` 的實作，以及分頁函式的實作）；`src/ui/View.cpp`（`RenderOverlays` 呼叫 `DrawInventory`）；`tests/ui/test_inventory_view.cpp`（單元測試分頁計算與渲染）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（ui domain），在 `RenderOverlays` 階段當 `inventoryOpen_` 時呼叫。純渲染，不驅動遊戲邏輯。

## OO 概念與設計重點

`DrawInventory` 只接收 DTO 而非 `World&` 是 **MVC 純度** 的典型應用：View 函式的輸入集合越小越精確，其可測性越高，與模型的耦合越低，對應 [arch-mvc](../concepts/arch-mvc.md)。`InventoryPageCount` 與 `InventoryPageOf` 從渲染函式中抽出，讓分頁計算邏輯可以單獨以純計算測試驗證，是 **關注點分離（SoC）** 的良好應用。透過 `IRenderer` 注入的渲染方式讓整個物品欄渲染在無 GL 環境的無頭測試中可執行，符合 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/InventoryView.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/InventoryView.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
