---
id: "file:tests/ui/test_inventory_view.cpp"
type: test
path: tests/ui/test_inventory_view.cpp
domain: tests
bucket: ui
loc: 505
classes: [Spy, Case]
sources: ["tests/ui/test_inventory_view.cpp"]
---
# `test_inventory_view.cpp`

> **一句定位**：驗證 `DrawInventory`（繪圖契約）與 `BuildInventoryRows`（資料組裝契約）的完整語意，涵蓋游標、分頁、說明換行、傘外觀色塊與第三章食物色塊。

## 職責

本測試是物品欄 UI 的主要測試檔，以 `Spy` IRenderer 攔截繪圖呼叫並斷言輸出。

**DrawInventory 繪圖契約**（共 9 個 TEST_CASE）：
- 背景、面板、標題、每列文字、游標符號（`> ` vs `  `）與數量後綴（`x2`、`x1`）正確。
- `count=0` 的列（傘類）不顯示 `xN` 後綴。
- 空背包顯示「（空）」佔位字。
- 游標索引超出範圍會被夾住，不崩潰。
- 被選取列顯示說明文字；可用消耗品顯示「E 使用」提示，僅供檢視的列（金幣）僅顯示「↑↓ 選擇」。
- 多分類背包矩形數量明顯多於純背景（每列有色塊）。

**分頁視窗（純函式）**：`InventoryPageCount`（空→1、整頁→1、多一個→2）、`InventoryPageOf`（游標所在頁，超出範圍夾住）。多頁時顯示「第 N／M 頁　←／→ 翻頁」；單頁時顯示「第 1／1 頁」但不顯示翻頁提示。

**BuildInventoryRows 資料組裝**：依手持傘種類（`HeldUmbrellaKind()`）而非結局旗標產生傘列；失去傘（`SetHasUmbrella(false)`）後傘列消失，即使結局旗標仍存在；5 種 `HeldUmbrella`（True/Cursed/Ugly/Fragile/ProfessorTrap）各對應唯一的道具表 id 與非空名稱/說明；None/Victim 不產生手持種類的傘列。

**色塊正確性**：真傘→藍、詛咒傘→暗紫、醜傘→螢光綠；破傘（Fragile）→ FragileBroken 灰；陷阱傘（ProfessorTrap）→危險紅；第三章香腸/大聲公用食物色塊（RGB 225,140,55），不可出現消耗品的青色瓶身（RGB 60,200,180）。

**說明換行**：長說明依 `CellWidth` 拆成多列，每列寬度 ≤ 54 字寬（DrawInventory 的換行上限）。

## 關鍵內容（類別 / 函式 / 資料）

- `Spy`（local struct）：實作 `IRenderer`，記錄 `rects`、`rectColors`、`texts`。
- `Find(rows, itemId)` / `Has(texts, str)` / `HasRectRGB(Spy, Color)`：輔助函式。
- `DrawInventory(IRenderer, rows, cursor, w, h)` — 被測繪圖函式。
- `BuildInventoryRows(Player)` — 被測資料組裝函式。
- `InventoryPageCount(total)` / `InventoryPageOf(cursor, total)` — 被測純函式。
- `HeldUmbrellaCatalogId(HeldUmbrella)` — 被測：種類→道具 id 對應；None/Victim 回傳 nullptr。
- `nccu::kInventoryRowsPerPage` — 每頁列數常數。
- `kItemMoney`/`kItemTrueUmbrella`/`kItemCursedUmbrella`/`kItemUglyUmbrella`/`kItemFragileUmbrella`/`kItemProfTrapUmbrella`/`kItemForm`/`kItemNotes`/`kItemVictimUmbrella`/`kItemSausage`/`kItemLoudspeaker`/`kItemLoanerUmbrella` — 道具 id 常數。

## 相依與在架構中的位置

- **#include（往外）**：`ui/InventoryView.h`（受測主體）、`engine/render/IRenderer.h`（Spy 基底）、`game/entities/Player.h`、`game/quest/Flags.h`、`game/quest/ItemCatalog.h`、`game/quest/Chapter1Quest.h`、`game/quest/Chapter2Quest.h`、`game/gfx/UmbrellaGlyph.h`、`game/dialog/DialogLayout.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 實作 `include/engine/render/IRenderer.h`。
- **每幀管線 / MVC 角色**：View 層測試，驗證 `DrawInventory` 的多個 Model 讀取路徑。

## OO 概念與設計重點

以 **Spy 模式** 注入 [IRenderer](../concepts/arch-dip-renderer.md) 達成無頭測試。`BuildInventoryRows` 以手持傘種類（`HeldUmbrellaKind`）而非持久旗標作為唯一真實來源，體現了 **Single Source of Truth** 原則，並防止「傘失去後舊列殘留」的過時狀態問題。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_inventory_view.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_inventory_view.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
