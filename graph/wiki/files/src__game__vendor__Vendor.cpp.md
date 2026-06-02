---
id: file:src/game/vendor/Vendor.cpp
type: source
path: src/game/vendor/Vendor.cpp
domain: game
bucket: vendor
loc: 110
classes: []
sources: ["src/game/vendor/Vendor.cpp"]
---
# `Vendor.cpp`

> **一句定位**：`Vendor`（繼承 `NPC`）的建構與 `TryBuy` 交易實作，處理「有限庫存、餘額檢查、傘型特例、業力鉤子、旗標觸發」五道交易守衛。

## 職責

此檔屬於 game / vendor 層，實作兩個函式：`BuildDialogLines` 和 `TryBuy`，以及 `Vendor` 的建構式。

`BuildDialogLines` 把 `VendorConfig` 轉為 `NPC::Interact` 可循環的對話行清單：若 `config.greetingLines` 非空則逐行展開（解析後的市集攤位帶有多行招呼語），否則退回單行 `greeting`；隨後每個庫存品項以 `FormatStockLine`（格式 `"<itemId> - <price> 元"`）接在後面。

建構式將上述對話行傳入 `NPC(position, lines, isQuestGiver=false, config.npcId)`，並把 `config_` 移入成員。

`TryBuy(Player*, std::size_t stockIndex)` 是交易的核心把關者，依序執行：
1. null 玩家或超出範圍的索引 → 靜默返回 false。
2. `item.stockLeft == 0`（售完）→ 發布「已售完」訊息，返回 false。
3. `player->DeductMoney(item.price)` 失敗（餘額不足）→ 發布「金幣不足」訊息，返回 false。
4. 成功：發布「買了 X，花了 N 元（剩 M 元）」提示橫幅（用 `ItemInfoFor` 取得中文名稱）+ `PickupAcquired` 事件；若 itemId 是雨傘型則 `SetHeldUmbrella(k)` 而非 `AddConsumable`；業力鉤子（`karmaOnInteract`）；有限庫存遞減；選用的 `setsFlag`（醜傘設下 `Flag_BoughtUglyUmbrella`）。

## 關鍵內容（類別 / 函式 / 資料）

- `FormatStockLine(const VendorItem&)` — 匿名命名空間輔助，格式化庫存行文字（避免使用 sstream）。
- `Vendor::BuildDialogLines(const VendorConfig&)` — 靜態方法，組出 NPC 循環對話行。
- `Vendor::Vendor(Vec2, VendorConfig)` — 建構式；委派給 `NPC`，移入 `config_`。
- `Vendor::TryBuy(Player*, std::size_t)` — 交易把關函式；回傳 bool 表示交易是否成功，副作用：扣款、發事件、設旗標、調業力、遞減庫存。
- `item.stockLeft` — -1 = 無限；0 = 售完；> 0 = 有限且遞減。
- `item.setsFlag` — 選用的逐品項旗標（醜傘 = `Flag_BoughtUglyUmbrella`）。
- `config_.karmaOnInteract` — 每次成功購買後的業力增減（預設 0 = 空操作）。

## 相依與在架構中的位置

- **#include（往外）**：`Vendor.h`（類別宣告）、`EventBus.h` / `EventSink.h`（發布 ShowMessage / PickupAcquired）、`Player.h`（扣款、設旗標、設持有雨傘）、`VendorMessages.h`（訊息字串常數）、`ItemCatalog.h`（`ItemInfoFor`、`HeldUmbrellaForItemId`）、`Color.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `WorldSpawn.cpp` 建立 `Vendor` 物件，由 `GameController` 在玩家與 `Vendor::NPC` 互動時呼叫 `TryBuy`。
- **繼承 / 實作 / 體現**：`Vendor` 繼承 `NPC`；`NPC` 繼承 `Character` / `GameObject`。
- **每幀管線 / MVC 角色**：Model 層物件（持有庫存狀態）；`TryBuy` 在 Controller 層的互動流程中觸發，直接修改 `Player` 狀態並發布事件。

## OO 概念與設計重點

`Vendor` 是 NPC 的子類別，體現了繼承與擴充（新增交易能力）而非改變 NPC 的對話機制。`TryBuy` 的「失敗快速返回」結構是防禦性程式設計的教科書範例：每道守衛在失敗時立即返回 false，成功路徑位於最後，副作用全部集中在確認交易成立後才執行。雨傘型 itemId 的特殊處理（`SetHeldUmbrella` 而非 `AddConsumable`）避免了「幻影第二列雨傘」的背包渲染問題，是「資料模型與呈現模型一致性」的邊界條件。[Observer](../concepts/pat-observer.md) 模式（EventBus）用於 UI 通知，保持 Vendor 不直接依賴 View。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/vendor/Vendor.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/vendor/Vendor.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [MVC](../concepts/arch-mvc.md)
