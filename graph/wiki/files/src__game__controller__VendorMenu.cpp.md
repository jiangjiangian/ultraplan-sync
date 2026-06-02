---
id: "file:src/game/controller/VendorMenu.cpp"
type: source
path: src/game/controller/VendorMenu.cpp
domain: game
bucket: controller
loc: 36
classes: []
sources: ["src/game/controller/VendorMenu.cpp"]
---
# `VendorMenu.cpp`

> **一句定位**：將攤販庫存轉換為對話選項（DialogState 的選項清單）的純組裝函式，統一商店 UI 的開啟邏輯。

## 職責

`OpenVendorMenu(DialogState& dlg, const Vendor& vendor)` 以 `VendorConfig` 的資料組裝並開啟一個攤販對話框：

1. **問候語行**：若 `cfg.greetingLines` 非空則使用多行問候，否則以 `cfg.greeting` 單行建立。
2. **庫存選項**：走訪 `cfg.stock`，每項建立一個 `DialogChoice`，標籤格式為 `"<id> - <price> 元"`；`karmaDelta=0`、`setsFlag=""`——選項不帶副作用（副作用全在 `Vendor::TryBuy`）。售罄項目仍顯示（`TryBuy` 回覆「賣完了」），使庫存數量可見。
3. **放棄選項**：恆在庫存之後附加 `kVendorDeclineLabel` 選項，索引等於 `cfg.stock.size()`，確保庫存索引 = 庫存槽位的對齊契約（`TryBuy(stockIdx)` 依賴此約定）。即使庫存為空也存在，避免無選項的死路。
4. 呼叫 `dlg.Open(greeting, choices)` + `dlg.SetNpcContext(kVendorContext)`（標記對話框所屬攤販，供 `HandleDialog` 識別攤販分支）。

## 關鍵內容（類別 / 函式 / 資料）

- `OpenVendorMenu(DialogState& dlg, const Vendor& vendor)` — 組裝問候語 + 庫存 DialogChoice + 放棄選項；呼叫 `dlg.Open` + `SetNpcContext`。
- `kVendorDeclineLabel`（引自標頭）— 放棄選項標籤字串（如「不買」）。
- `kVendorContext`（引自標頭）— 攤販對話框的 NpcId 標記字串。
- 庫存選項格式：`item.itemId + " - " + to_string(item.price) + " 元"`。

## 相依與在架構中的位置
- **#include（往外）**：`VendorMenu.h`；`DialogState.h`（`DialogChoice`、`dlg.Open`）；`Vendor.h`（`VendorConfig`、`stock`）
- **被誰使用（往內）**：—（由 `InteractDispatch.cpp` 的 `DispatchInteract` 呼叫，並被 `DialogScreen.cpp` 的 `kVendorContext` 識別）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；在 `DispatchInteract`（E 鍵分派）的 Vendor 分支中呼叫

## OO 概念與設計重點

`OpenVendorMenu` 將「商店 UI 組裝」從 `GameController` 抽出（SRP），使 `InteractDispatch`、`DialogScreen`、`VendorMenu` 三個模組各自聚焦。庫存選項不帶副作用（`karmaDelta=0`、`setsFlag=""`）的設計保持 `DialogChoice` 的通用性，所有商業邏輯（金錢扣除、物品加入、事件發布）完全封裝在 `Vendor::TryBuy` 內——是資料與行為分離（DDD Repository 精神）的輕量版應用。「放棄選項恆在最後、索引 = stockSize」的設計是 `HandleDialog` 「最後一個選項 = 不買」偵測的契約基礎，是一個精心維護的介面不變式。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/VendorMenu.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/VendorMenu.cpp) · [← 全檔索引](../files-index.md)
