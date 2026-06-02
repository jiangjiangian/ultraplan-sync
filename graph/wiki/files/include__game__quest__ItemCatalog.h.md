---
id: file:include/game/quest/ItemCatalog.h
type: header
path: include/game/quest/ItemCatalog.h
domain: game
bucket: quest
loc: 157
classes: [ItemInfo, InventoryRow]
sources: ["include/game/quest/ItemCatalog.h"]
---
# `ItemCatalog.h`

> **一句定位**：玩家可持有之每件物品的程式目錄與背包 DTO 組裝介面，是 Tab 背包顯示名稱、描述與可用性判斷的唯一真實來源。

## 職責

`ItemCatalog.h` 定義兩個結構體（`ItemInfo`、`InventoryRow`）與一組純查詢/組裝函式，集中管理所有可持有物品的中文顯示名、描述、可使用性判斷，以及從 `Player` 組裝整個背包 DTO 的 `BuildInventoryRows`。

檔案宣告了完整的物品哨兵 itemId 集合，涵蓋非消耗品的所有持有物：貨幣（`kItemMoney`）、六種傘（`kItemTrueUmbrella`、`kItemCursedUmbrella`、`kItemUglyUmbrella`、`kItemVictimUmbrella`、`kItemFragileUmbrella`、`kItemProfTrapUmbrella`）、借傘（`kItemLoanerUmbrella`）、任務紙張（`kItemForm`、`kItemNotes`）以及 Ch3 物物交換鏈攜帶物（`kItemSausage`、`kItemLoudspeaker`）。這些哨兵 id 以雙底線包圍，確保不與 Vendor 庫存的正規 itemId 碰撞。

函式組涵蓋：`HeldUmbrellaCatalogId`（從 `HeldUmbrella` 枚舉對應到哨兵 id）、`HeldUmbrellaForItemId`（反向對應，購買傘時轉為手持傘種類）、`ItemInfoFor`（取中文名+描述，未知 id 不丟例外回退到 id 本身）、`CatalogStrings`（枚舉所有文案供字形掃描）、`IsUsableConsumable`（判定是否可從背包使用）、`IsUmbrellaItemId`（判定是否傘品，供 `BuildInventoryRows` 排除計數迴圈）、`ApplyConsumableEffect`（從背包使用時套用效果，與地面拾取的 `Consume` 共用常數）以及 `BuildInventoryRows`（從 Player 組裝有序的背包列 DTO 向量）。

`BuildInventoryRows` 的順序確定：金幣→依 itemId 排序的消耗品→傘→任務紙張，使背包面板與回歸測試結果穩定。

## 關鍵內容（類別 / 函式 / 資料）

- `struct ItemInfo`：顯示名 `displayName`（`string_view`）與描述 `description`（`string_view`）。
- `struct InventoryRow`：背包列 DTO，含中文名 `name`、數量 `count`、描述 `description`、可用性 `usable`（bool）、物品 id `itemId`。
- 物品哨兵常數（`kItemMoney`、`kItemTrueUmbrella` 等 12 個 `inline constexpr const char*`）。
- `HeldUmbrellaCatalogId(HeldUmbrella) → const char*`：枚舉→哨兵 id，None/Victim 回傳 nullptr。
- `HeldUmbrellaForItemId(string_view) → HeldUmbrella`：itemId→手持傘種類，非傘 id 回傳 None。
- `ItemInfoFor(string_view) → ItemInfo`：取目錄條目，未知 id 有安全退路。
- `CatalogStrings() → vector<string>`：枚舉所有文案，供字形覆蓋掃描。
- `IsUsableConsumable(string_view) → bool`：背包「使用」可用性守衛。
- `IsUmbrellaItemId(string_view) → bool`：含子字串 "Umbrella"/"umbrella" 判定。
- `ApplyConsumableEffect(EventBus&, Player&, string_view)`：從背包使用消耗品效果，不扣計數（由呼叫端 `ConsumeOne`）。
- `BuildInventoryRows(const Player&) → vector<InventoryRow>`：組裝整個背包 DTO。

## 相依與在架構中的位置

- **#include（往外）**：`Player.h`（`HeldUmbrella` 枚舉、`Player` 讀取旗標與計數表）
- **被誰使用（往內）**：`include/ui/InventoryView.h`、`src/game/controller/GameController.cpp`、`src/game/controller/screens/InventoryScreen.cpp`（背包畫面）、`src/game/quest/Chapter1Quest.cpp`、`src/game/vendor/Vendor.cpp`（購買提示文案）、`src/ui/InventoryView.cpp`；多個測試（`test_inventory_view.cpp`、`test_vendor_inventory.cpp`、`test_consumable.cpp` 等）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model/game 層——`BuildInventoryRows` 由 InventoryScreen（Controller）呼叫後傳給 View 渲染；`ApplyConsumableEffect` 由 Controller 在「從背包使用」動作後呼叫。

## OO 概念與設計重點

本檔是**單一責任原則（SRP）**的積極體現：把物品文案從遊戲邏輯中抽離，使「顯示名更新」和「使用效果調整」可分別修改而不互相污染。`InventoryRow` 作為純 DTO（Data Transfer Object），確保 View 只需渲染，不持有 World/Player 控柄，符合 [MVC](../concepts/arch-mvc.md) 的分層要求。`CatalogStrings()` 的字形掃描機制是一種靜態驗證手段：任何新增文案未烘進字型都會在測試期被發現（使測試建立為強驗證屏障）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ItemCatalog.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ItemCatalog.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
