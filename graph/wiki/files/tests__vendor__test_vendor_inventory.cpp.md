---
id: "file:tests/vendor/test_vendor_inventory.cpp"
type: test
path: tests/vendor/test_vendor_inventory.cpp
domain: tests
bucket: vendor
loc: 154
classes: [MsgCapture]
sources: ["tests/vendor/test_vendor_inventory.cpp"]
---
# `test_vendor_inventory.cpp`

> **一句定位**：驗證 Vendor 購買的延伸欄位——計數背包、業力鉤子、有限庫存售罄，以及購買提示顯示中文名稱而非 itemId。

## 職責

本測試是 `test_vendor.cpp` 的補充，驗證較新加入的 `VendorConfig` 欄位和 `Player` 計數背包。

**Player 計數背包**：`AddConsumable(id)` 鏈式調用；`ConsumableCount(id)` 正確計數；`ConsumeOne(id)` 每次減 1，耗盡後回傳 false；未知 id 也回傳 false。

**TryBuy 放進計數背包**：成功購買後 `ConsumableCount("HotPack")` 從 0 增加；多次購買正確累積（購買 2 次 → count=2）。

**業力鉤子（karmaOnInteract）**：`VendorConfig::karmaOnInteract = 1` 的募款箱，`TryBuy` 成功後 `Player::GetKarma()` 增加 1；`karmaOnInteract = 0` 的一般攤位不改變業力。

**有限庫存（stockLeft）**：`stockLeft = 2` 的限量攤；第 1、2 次購買成功並扣款；第 3 次購買 `TryBuy` 回傳 false，不扣款，`ConsumableCount` 不增加，`lastMsg == kSoldOut`。

**購買提示格式**：醜傘（`UglyUmbrella`，100 元）購買提示為「買了螢光綠醜傘，花了 100 元（剩 0 元）」，同時設定 `kFlagBoughtUglyUmbrella`。

**全 itemId 有中文名**：10 個常用 itemId（HotPack、EnergyDrink、WaterproofSpray、EggCake、FlowerTea、Takoyaki、Donation、UglyUmbrella、CursedUmbrella、TransparentUmbrella）的 `ItemInfoFor(id).displayName` 非空且不等於 id 本身。

## 關鍵內容（類別 / 函式 / 資料）

- `MsgCapture`：`EventBus::Instance().Clear()` 並訂閱 `ShowMessage`，記錄 `msgHits` 與 `lastMsg`。
- `Player::AddConsumable(id)` / `ConsumableCount(id)` / `ConsumeOne(id)` — 被測背包 API。
- `VendorConfig::karmaOnInteract` — 業力鉤子欄位。
- `VendorItem::stockLeft` — 有限庫存欄位（-1 為無限）。
- `nccu::ItemInfoFor(id)` — 被測：回傳 `displayName`（中文名稱）。
- `nccu::vendor::msg::kSoldOut` — 售罄提示常數。

## 相依與在架構中的位置

- **#include（往外）**：`engine/events/EventBus.h`、`game/entities/Player.h`、`game/quest/Flags.h`、`game/quest/ItemCatalog.h`、`game/vendor/Vendor.h`、`game/vendor/VendorConfig.h`、`game/vendor/VendorMessages.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試，覆蓋 Player 背包狀態與 Vendor 的延伸購買路徑。

## OO 概念與設計重點

以 `MsgCapture` 訂閱 [EventBus](../concepts/pat-observer.md) 事件觀察側效果，不需直接訪問 Vendor/Player 私有狀態。`ItemInfoFor` 的全覆蓋測試確保背包列與購買提示永遠顯示中文，不洩漏英文 itemId 到 UI 層。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/vendor/test_vendor_inventory.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/vendor/test_vendor_inventory.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
