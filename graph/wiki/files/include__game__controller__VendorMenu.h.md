---
id: "file:include/game/controller/VendorMenu.h"
type: header
path: include/game/controller/VendorMenu.h
domain: game
bucket: controller
loc: 53
classes: []
sources: ["include/game/controller/VendorMenu.h"]
---
# `VendorMenu.h`

> **一句定位**：把 `Vendor` 庫存組成「購買選單」對話的建構器，以及辨識商店對話/放棄選項所需的兩個哨兵字串常數。

## 職責

`VendorMenu.h` 是 game controller 層的商店選單接縫，宣告了一個自由函式 `OpenVendorMenu` 和兩個 `inline constexpr` 哨兵字串，解決「如何把 Vendor 的庫存轉成 DialogState 格式」的問題。

`OpenVendorMenu(DialogState& dlg, const Vendor& vendor)` 組出商店對話：問候語作為開場台詞，每件在庫商品一個 `DialogChoice`（標籤沿用 Vendor 自身的庫存格式化字串），最後追加一個「先不買，謝謝」（`kVendorDeclineLabel`）的放棄選項。設計要點：`DialogChoice` 本身不帶業力/旗標，所有經濟副作用（`DeductMoney`、`AddConsumable`、EventBus 購買事件、金錢軟上限、`item.setsFlag`）一律留在 `Vendor::TryBuy` 內；庫存選項只攜帶其在 `Choices()` 中的索引位置，確認後由 `HandleDialog` 導向 `Vendor::TryBuy`。

**`kVendorContext = "__vendor__"`**：商店對話的哨兵 NPC 情境字串。因 `Vendor` 的 `NpcId()` 為空，其開啟的對話以此哨兵標記，使 `HandleDialog` 確認分支能以 `dlg.NpcId() == kVendorContext` 判定「確認的是庫存項目」而非 NPC 對白選項，進而導向 `Vendor::TryBuy`。

**`kVendorDeclineLabel = "先不買，謝謝"`**：放棄選項的標籤。靠「選中索引 == 庫存數」（恆為最後一個選項）辨識為放棄，確認後關閉對話且不呼叫 `Vendor::TryBuy`，保證購買永不可被強迫。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `kVendorContext = "__vendor__"` | 商店對話的哨兵 NPC 情境字串；`HandleDialog` 用它辨識庫存確認路徑。 |
| `kVendorDeclineLabel = "先不買，謝謝"` | 放棄選項的固定標籤；靠位置（最後一個）辨識，不帶任何業力/旗標 payload。 |
| `OpenVendorMenu(DialogState& dlg, const Vendor& vendor)` | 自由函式；把問候語+庫存+放棄選項組入 `dlg`，供 `HandleDialog` 選項確認路徑使用。 |

## 相依與在架構中的位置

- **#include（往外）**：僅 `<string_view>`；`Vendor` 和 `DialogState` 為前向宣告。最小相依。
- **被誰使用（往內）**：`src/game/controller/InteractDispatch.cpp`（E 鍵觸碰 Vendor 時呼叫 `OpenVendorMenu`）、`src/game/controller/VendorMenu.cpp`（`OpenVendorMenu` 實作）、`src/game/controller/screens/DialogScreen.cpp`（讀取兩個哨兵常數以分流確認邏輯）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層的商店互動工具；在 E 鍵觸碰 Vendor（`DispatchInteract` 階段）時呼叫，也在 `HandleDialog` 確認分支時以哨兵常數判斷流向。

## OO 概念與設計重點

`kVendorContext` 和 `kVendorDeclineLabel` 是「哨兵值（Sentinel Value）」設計模式的應用：用一個特殊的已知字串標記特殊狀態，讓協定（Protocol）不需額外的型別或旗標就能表達「這是商店對話」和「這是放棄」兩個特殊條件，避免了在 `DialogState` 中引入商店特有的欄位。

把所有經濟副作用留在 `Vendor::TryBuy`（而非 `DialogChoice` 的 payload）是 **SRP** 的直接體現：選單構建（`OpenVendorMenu`）與購買執行（`TryBuy`）是兩個不同的職責，清晰分離。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/VendorMenu.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/VendorMenu.h) · [← 全檔索引](../files-index.md)
