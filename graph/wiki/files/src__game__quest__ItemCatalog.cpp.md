---
id: file:src/game/quest/ItemCatalog.cpp
type: source
path: src/game/quest/ItemCatalog.cpp
domain: game
bucket: quest
loc: 297
classes: []
sources: ["src/game/quest/ItemCatalog.cpp"]
---
# `ItemCatalog.cpp`

> **一句定位**：遊戲中所有可持有物品的「圖鑑資料庫」與背包列建構器，是背包 DTO（`InventoryRow`）資料的唯一事實來源。

## 職責

此檔屬於 game / quest 層，負責三件相互關聯的事：第一，以一張靜態的 `std::unordered_map<std::string, ItemInfo>` 表格（匿名命名空間中的 `Table()`）收錄所有 itemId 對應的中文顯示名稱與圖鑑說明，涵蓋貨幣、三種主線消耗品、市集小吃、各型雨傘哨兵值與任務紙張；第二，提供幾個輔助純函式（`IsUsableConsumable`、`IsUmbrellaItemId`、`HeldUmbrellaForItemId`、`HeldUmbrellaCatalogId`）做分類判斷，供 `InventoryView` 與 `Vendor::TryBuy` 共用；第三，在 `BuildInventoryRows` 中把 `Player` 的即時持有狀態（金錢、計數消耗品、持有型雨傘、旗標驅動的任務攜帶物）依照嚴謹的規則組裝成 `std::vector<InventoryRow>`，供背包畫面直接渲染。

`ApplyConsumableEffect` 是另一個關鍵函式，它把玩家從背包「使用」消耗品的效果鏡像（完全一致地複製）三種主線消耗品類別（`EnergyDrink`、`HotPack`、`WaterproofSpray`）的 `Consume` 主體，並以 `EnergyDrink::kKarmaBonus` / `HotPack::kKarmaBonus` / `WaterproofSpray::kRainRelief` 等類別常數來保證數值永不分叉。市集小吃（`EggCake`、`FlowerTea`、`Takoyaki`）無對應實體類別，其 -15 雨量效果唯一記載於此。

背包列建構規則特別細膩：雨傘型 itemId（任何包含 "Umbrella" / "umbrella" 子字串者）從計數消耗品迴圈排除，改由 `HeldUmbrellaKind()` 以「即時持有種類」的單一持有型雨傘列呈現；苦主透明傘（`kFlagHasVictimUmbrella`）、圖書館管理員借傘（`kFlagLibrarianUmbrella`）、申請書（`kFlagFoundForm`）、學霸筆記（三頁旗標合計一列計數）、第三章物物交換道具（香腸 / 大聲公）皆以旗標驅動獨立列，不走計數路徑。

## 關鍵內容（類別 / 函式 / 資料）

- `Table()` — 函式區域 static 的 `unordered_map<string, ItemInfo>`，所有 itemId 的中文名稱與圖鑑描述之唯一來源。
- `ItemInfoFor(string_view)` — 按 itemId 查詢圖鑑列，不存在時以 itemId 本身為顯示名稱退路。
- `CatalogStrings()` — 匯出所有顯示名稱與描述字串，供字形掃描測試（`test_font_ui_glyph_scan`）驗證圖集完整性。
- `IsUsableConsumable(string_view)` — 判斷 itemId 是否可從背包使用（三種消耗品 + 三種市集小吃；愛心捐款、金幣、雨傘、紙張均排除）。
- `IsUmbrellaItemId(string_view)` — 以子字串比對判斷是否為雨傘類 itemId，為 `BuildInventoryRows` 與 `InventoryView` 的共用述詞。
- `HeldUmbrellaForItemId(string_view)` — 攤販庫存的雨傘 itemId → `HeldUmbrella` 列舉（供 `Vendor::TryBuy` 呼叫）。
- `HeldUmbrellaCatalogId(HeldUmbrella)` — 持有型雨傘種類 → 圖鑑哨兵 itemId（供 `BuildInventoryRows` 查詢）。
- `ApplyConsumableEffect(EventBus&, Player&, string_view)` — 從背包使用消耗品的效果執行點；鏡像各 `ConsumableItem::Consume` 主體，並以 `EventType::ShowMessage` 發布提示。
- `PushRow(...)` — 匿名命名空間輔助函式，組裝 `InventoryRow` 並附加到 `rows`。
- `BuildInventoryRows(const Player&)` — 遊戲中唯一把 `Player` 狀態轉換為背包 DTO 的函式，回傳 `vector<InventoryRow>`。
- `kFoodRainRelief` — 市集通用小吃的雨量緩解常數（15.0f）。

## 相依與在架構中的位置

- **#include（往外）**：`ItemCatalog.h`（圖鑑 API 宣告）、`Player.h`（讀業力 / 金錢 / 消耗品 / 旗標 / 持有雨傘）、`EnergyDrink.h` / `HotPack.h` / `WaterproofSpray.h`（常數 `kKarmaBonus` / `kRainRelief`，確保效果值單一來源）、`Chapter1Quest.h` / `Chapter2Quest.h` / `Chapter3Quest.h`（`kFlag*` 旗標常數）、`EventBus.h`（`ApplyConsumableEffect` 發布 `ShowMessage`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；被 `View.cpp`（呼叫 `BuildInventoryRows`）、`Vendor.cpp`（呼叫 `ItemInfoFor`、`HeldUmbrellaForItemId`）、`InventoryView.cpp`（呼叫 `IsUmbrellaItemId`）、測試等 include `ItemCatalog.h` 的編譯單元所用。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬 Model 層資料服務（game/quest），不直接參與每幀管線；`BuildInventoryRows` 在 `View::RenderOverlays` 每幀呼叫，屬 View 從 Model 讀取資料的橋樑。

## OO 概念與設計重點

檔案以函式區域 static 的 `Table()` 實作典型的「懶惰初始化單例資料表」（與 [Singleton](../concepts/pat-singleton.md) 同手法），確保所有 `string_view` 指向靜態表格、生命週期安全。

`ApplyConsumableEffect` 刻意做到與三個 `ConsumableItem` 子類別的 `Consume` 完全一致，展現了對 [Template Method](../concepts/pat-template.md) 效果路徑的「鏡像複製」設計取捨——代價是兩條路徑需靠測試維持同步，換來不需任何 `dynamic_cast` 或虛擬函式呼叫（符合 [ISP / Roles](../concepts/oo-isp-roles.md) 的無 RTTI 精神）。

`IsUmbrellaItemId` 做為 `BuildInventoryRows` 與 `InventoryView` 共用的唯一述詞，體現了 DRY 原則——僅靠子字串比對、不耦合任何類別，能穩健覆蓋現有及未來的雨傘 itemId。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/ItemCatalog.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ItemCatalog.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Singleton](../concepts/pat-singleton.md) · [Template Method](../concepts/pat-template.md) · [ISP / Roles](../concepts/oo-isp-roles.md)
