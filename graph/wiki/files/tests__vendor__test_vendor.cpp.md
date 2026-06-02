---
id: "file:tests/vendor/test_vendor.cpp"
type: test
path: tests/vendor/test_vendor.cpp
domain: tests
bucket: vendor
loc: 139
classes: [VendorCapture]
sources: ["tests/vendor/test_vendor.cpp"]
---
# `test_vendor.cpp`

> **一句定位**：驗證 `Vendor` 的核心購買契約——對話列建構、`TryBuy` 成功/失敗路徑及 Factory 建構。

## 職責

本測試固定 `Vendor` 類別的基本行為合約，以 `VendorCapture`（手動訂閱 `ShowMessage` 和 `PickupAcquired` 事件）觀察副作用。

**建構**：`Vendor(pos, config)` 由 greeting + 一個庫存項建出 2 條對話列；`CurrentLineText()` 為 greeting；`Config().name`、`Config().stock` 與 `VendorConfig` 一致。

**TryBuy 成功**：扣款（money - price）；發出恰好一個 `ShowMessage`（文字為「買了{中文名}，花了 N 元（剩 M 元）」，使用 catalog 中文名而非 itemId）；發出恰好一個 `PickupAcquired`（text 為 itemId）；回傳 true。

**TryBuy 金錢不足**：不扣款；`ShowMessage` 文字為「你錢不夠」；無 `PickupAcquired`；回傳 false。以 `DeductMoney(1)` 迴圈把錢包清空到 0 來設置 fixture。

**TryBuy 索引越界**：索引 1 和 99（庫存大小為 1）皆回傳 false，不發任何事件，不改變金錢。

**TryBuy null player**：回傳 false，不發任何事件（防崩潰保護）。

**Factory**：`GameObjectFactory::Create(ObjectType::Vendor, {1,2})` 回傳非 null、可 `dynamic_cast<Vendor*>` 成功；`Config().name == "市集攤主"`；`Config().stock.size() == 1`。

## 關鍵內容（類別 / 函式 / 資料）

- `VendorCapture`：手動呼叫 `EventBus::Instance().Clear()` 清空並訂閱 `ShowMessage`/`PickupAcquired`，記錄 `msgHits`、`lastMsg`、`pickupHits`、`lastPickup`。
- `MakeOneItemStall()`：建立含一個 HotPack（30 元）庫存的 `VendorConfig` fixture。
- `Vendor::TryBuy(Player*, int stockIdx)` — 被測主要函式。
- `Vendor::DialogLineCount()` / `CurrentLineText()` / `Config()` — 被測查詢函式。
- `GameObjectFactory::Create(ObjectType::Vendor, Vec2)` — 被測 Factory。

## 相依與在架構中的位置

- **#include（往外）**：`engine/events/EventBus.h`、`game/controller/GameObjectFactory.h`、`game/entities/Player.h`、`game/vendor/Vendor.h`、`game/vendor/VendorConfig.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試，驗證 Vendor 實體的購買邏輯及 [Factory Method](../concepts/pat-factory.md) 建構路徑。

## OO 概念與設計重點

透過 [Observer / EventBus](../concepts/pat-observer.md) 觀察購買副作用，不需直接訪問 Vendor 私有狀態。[Factory Method](../concepts/pat-factory.md) 測試確認 `GameObjectFactory` 能正確路由到 Vendor 子類別並初始化 config。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/vendor/test_vendor.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/vendor/test_vendor.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [Factory](../concepts/pat-factory.md)
