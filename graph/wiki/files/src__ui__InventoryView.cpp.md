---
id: file:src/ui/InventoryView.cpp
type: source
path: src/ui/InventoryView.cpp
domain: ui
bucket: 
loc: 257
classes: []
sources: ["src/ui/InventoryView.cpp"]
---
# `InventoryView.cpp`

> **一句定位**：背包畫面的完整渲染——分頁列表、左緣分類色塊（傘 / 金幣 / 紙張 / 食物 / 消耗品）、被選列說明帶，以及分頁輔助函式 `InventoryPageCount` / `InventoryPageOf`。

## 職責

此檔屬於 ui 層，從 `vector<InventoryRow>` DTO（由 `ItemCatalog::BuildInventoryRows` 建構）繪製背包 UI，完全不觸碰 `World` / `Player`（MVC 純度）。

**分頁輔助函式**（宣告於標頭、有單元測試）：`InventoryPageCount(rowCount)` — 無條件進位取頁數（空背包為 1 頁）；`InventoryPageOf(cursor, rowCount)` — 由游標衍生頁索引（不保留 UI 狀態，故存檔逐位元一致）。

**`RowKind` enum（匿名命名空間）**：`Consumable / Money / Umbrella / Paper / Food`，由 `KindOf(row)` 以 `itemId` 純推導。`Food` 類別是為第三章香腸 / 大聲公而設，防止它們誤用青色藥水瓶色塊（那暗示「可用」）。

**`UmbrellaLookOf(row)`**：每種持有型雨傘哨兵值 → 對應外觀（`CursedPurple` / `UglyGreen` / `FragileBroken` / `ProfessorTrap` / `TrueBlue`），修復舊版所有非 cursed/ugly 傘都畫完好藍色的問題。

**`DrawSwatch(r, row, kind, box)`**：左緣 20×20 色塊的具體繪製邏輯，依 `RowKind` 分支：雨傘用 `DrawUmbrellaGlyph`、金幣畫圓形標記、紙張畫折角白紙、食物畫暖橘包裹 + 綁帶、消耗品畫青色藥水瓶。

**`DrawInventory(r, rows, cursor, W, H)`**：全螢幕暗化疊層 → 468×372 面板 → 標題 → 分頁列表（每列：選取條、色塊、「> 」/ 「  」前綴、名稱 + xN）→ 頁碼指示（多頁時附 ←/→ 提示）→ 說明帶（WrapToCells + 54 字格換行 + 使用提示）。

## 關鍵內容（類別 / 函式 / 資料）

- `InventoryPageCount(int)` / `InventoryPageOf(int, int)` — 公開純函式，有單元測試。
- `RowKind` — 匿名 enum；驅動色塊與文字顏色。
- `KindOf(const InventoryRow&)` — `itemId` → `RowKind` 分類函式（使用 `IsUmbrellaItemId`，與 `BuildInventoryRows` 共用）。
- `UmbrellaLookOf(const InventoryRow&)` — `itemId` → `UmbrellaLook` 外觀映射。
- `RowColor(RowKind)` — 各分類的文字顏色（金幣金 / 柔藍 / 紙白 / 暖褐 / 消耗品金）。
- `DrawSwatch(IRenderer&, InventoryRow&, RowKind, Rect)` — 左緣色塊繪製。
- `DrawInventory(IRenderer&, vector<InventoryRow>&, int cursor, float W, float H)` — 完整背包渲染。
- `kInventoryRowsPerPage` — 每頁行數常數（來自 `InventoryView.h`）。
- `kDescLines` = 3 — 說明帶最多顯示的換行行數。
- `kDescCells` = 54 — 說明文字換行的字格上限。

## 相依與在架構中的位置

- **#include（往外）**：`InventoryView.h`、`IRenderer.h`、`Rect.h`/`Vec2.h`/`Color.h`、`UmbrellaGlyph.h`（色塊 / 傘外觀）、`ItemCatalog.h`（`kItem*` 哨兵值、`IsUmbrellaItemId`）、`DialogLayout.h`（`WrapToCells`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderOverlays` 在 `world.InventoryOpen()` 時呼叫 `DrawInventory`；由單元測試呼叫 `InventoryPageCount` / `InventoryPageOf`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層疊層；在 `RenderOverlays` 末段繪製，疊在世界 / HUD / 對話之上。

## OO 概念與設計重點

分頁邏輯由游標完全衍生（`InventoryPageOf(sel, rowCount)`），不保留任何 UI 狀態，確保存檔逐位元不變，符合 [MVC](../concepts/arch-mvc.md) 的 View 無副作用設計。`KindOf` 與 `IsUmbrellaItemId` 共用 `ItemCatalog` 的同一述詞，體現單一事實來源。`UmbrellaLookOf` 修復舊版「所有傘都畫藍色」的問題，是「呈現外觀以身分（itemId）為鍵而非以類別繼承為鍵」的設計展示。`Food` 分類的引入防止道具被誤讀為可用消耗品，是「分類感知的 UX 防衛」。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/InventoryView.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/InventoryView.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
