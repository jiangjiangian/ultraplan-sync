---
id: file:include/game/vendor/Vendor.h
type: header
path: include/game/vendor/Vendor.h
domain: game
bucket: vendor
loc: 63
classes: [Vendor]
sources: ["include/game/vendor/Vendor.h"]
---
# `Vendor.h`

> **一句定位**：同時兼任商店櫃檯的 NPC 類別——以 `NPC::Interact()` 對話循環瀏覽商品，`TryBuy()` 作為實際成交的分離入口。

## 職責

`Vendor.h` 宣告 `Vendor final : public NPC`，繼承 NPC 的基本對話/移動功能，並加上攤販專屬的 `TryBuy`、`IsVendor()` 與 `Config()` 接口。

瀏覽與成交走兩條路徑、互不干擾：`NPC::Interact()` 驅動台詞循環讓玩家「看目錄」，而 `TryBuy(player, stockIndex)` 由商店畫面 UI 在玩家選定購買後帶著明確庫存索引呼叫，執行三種結果：索引越界→false（靜默失敗）、`DeductMoney` 失敗（錢不夠）→false 並發出 `ShowMessage "你錢不夠"`，成交→true 並發出 `ShowMessage` 購買確認與 `PickupAcquired` 事件。

`IsVendor()` 覆寫 `NPC` 的虛函式並恆回傳 true，供 `GameController` 的 E 互動分派以虛擬分派（而非 `dynamic_cast`）判斷是否導向購買選單 UI，而非 `NPC::Interact` 的台詞循環。`Config()` 以 const 參照回傳 `VendorConfig`，供 `VendorMenu` UI 讀取商品列表。

建構式接受 `Vec2 position` 與 `VendorConfig config`，並以私有靜態方法 `BuildDialogLines(config)` 從問候語與庫存項組出 NPC 對話台詞，建構時呼叫一次。

## 關鍵內容（類別 / 函式 / 資料）

- `class Vendor final : public NPC`：最終類別，不可再繼承。
- `Vendor(Vec2 position, VendorConfig config)`：建構式，呼叫 `BuildDialogLines` 初始化對話列。
- `TryBuy(Player* player, size_t stockIndex) → bool`：購買嘗試，三種結果（越界/錢不夠/成交）。
- `IsVendor() const noexcept → bool`：恆回傳 true，供 GameController 虛擬分派導向購買 UI。
- `Config() const noexcept → const VendorConfig&`：取攤販設定（供 VendorMenu 讀取商品列表）。
- `BuildDialogLines(const VendorConfig&) → vector<string>`（靜態私有）：由問候語與庫存項目組出台詞。
- `config_`（私有，`VendorConfig`）：攤販設定。

## 相依與在架構中的位置

- **#include（往外）**：`NPC.h`（基底類別）、`VendorConfig.h`（`VendorItem`/`VendorConfig` POD）、`Vec2.h`（建構位置參數）
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（E 互動分派 `IsVendor()`）、`src/game/controller/GameObjectFactory.cpp`（建構攤販物件）、`src/game/controller/InteractDispatch.cpp`、`src/game/controller/VendorMenu.cpp`（購買 UI 呼叫 `TryBuy`）、`src/game/controller/screens/DialogScreen.cpp`、`src/game/vendor/Vendor.cpp`（實作體）、`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`；測試（`test_roles.cpp`、`test_vendor.cpp`、`test_vendor_inventory.cpp`）
- **繼承 / 實作 / 體現**：繼承自 `NPC.h`
- **每幀管線 / MVC 角色**：game 層物件——Vendor 是 World 的物件容器成員，View 渲染它（繼承自 NPC→Character→GameObject 的 `IDrawable`），Controller 在 E 互動時以 `IsVendor()` 決策並呼叫 `TryBuy`。

## OO 概念與設計重點

`IsVendor()` 的虛擬分派設計是拒絕 `dynamic_cast` 的**開放封閉**體現：不需要感知具體型別，GameController 只依賴 NPC 介面上的虛函式，未來若新增其他特殊 NPC 子類別（如任務觸發型 NPC）只需覆寫對應的辨識函式。`final` 關鍵字明確表示 Vendor 不預期被繼承，防止未來不當擴展。瀏覽/成交路徑分離的設計讓「看商品」與「付款購買」分屬不同交互入口，避免合并帶來的狀態複雜性。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/vendor/Vendor.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/Vendor.h) · [← 全檔索引](../files-index.md)
