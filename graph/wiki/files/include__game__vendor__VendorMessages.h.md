---
id: file:include/game/vendor/VendorMessages.h
type: header
path: include/game/vendor/VendorMessages.h
domain: game
bucket: vendor
loc: 44
classes: []
sources: ["include/game/vendor/VendorMessages.h"]
---
# `VendorMessages.h`

> **一句定位**：Vendor 對外顯示的全部繁中文案常數，以 `string_view` 定義在獨立命名空間 `nccu::vendor::msg` 中，把文案與交易邏輯分離（SRP）。

## 職責

`VendorMessages.h` 把 `Vendor::TryBuy` 與 `NPC::BuildDialogLines` 所需的顯示字串集中定義為 `inline constexpr string_view` 常數，自 `Vendor.cpp` 抽出。設計動機是 SRP：文案（因劇情調整而改）與交易邏輯（因規則調整而改）兩者因不同理由變動，應分離。維持為 `string_view` 常數也讓日後 i18n 對照表可直接抽換。

命名空間 `nccu::vendor::msg` 提供：`kInsufficientFunds`（錢包不動、交易未發生）、成交提示的四個組件（`kPurchasedPrefix = "買了"`、`kSpentMid = "，花了 "`、`kSpentUnitOpen = " 元（剩 "`、`kSpentUnitClose = " 元）"`，組合成「買了<中文名>，花了 N 元（剩 N 元）」格式）、`kSoldOut`（有限庫存售罄），以及庫存對話行的兩個組件（`kStockLineSep = " - "`、`kStockLineUnit = " 元"`，格式為 `<itemId> - <price> 元`）。

## 關鍵內容（類別 / 函式 / 資料）

- `kInsufficientFunds = "你錢不夠"`：`DeductMoney` 失敗時顯示。
- `kPurchasedPrefix = "買了"` / `kSpentMid` / `kSpentUnitOpen` / `kSpentUnitClose`：成交確認訊息的四個片段常數。
- `kSoldOut = "賣完了"`：有限庫存售罄時顯示（stockLeft 歸 0，扣款前失敗）。
- `kStockLineSep = " - "` / `kStockLineUnit = " 元"`：庫存對話行組件（由 `BuildDialogLines` 使用）。

## 相依與在架構中的位置

- **#include（往外）**：`<string_view>`
- **被誰使用（往內）**：`src/game/quest/Chapter1Quest.cpp`（特定 Ch1 攤販訊息）、`src/game/vendor/Vendor.cpp`（`TryBuy` 與 `BuildDialogLines` 文案）；`tests/ui/test_font_ui_glyph_scan.cpp`（字形覆蓋掃描）、`tests/vendor/test_vendor_inventory.cpp`（庫存測試驗證訊息文案）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純常數定義；由 Vendor（Model/game 層）在購買流程中組合訊息後透過 EventBus 發布 `ShowMessage` 事件。

## OO 概念與設計重點

本檔體現了**單一責任原則（SRP）**的文案管理應用：把顯示文字集中到獨立標頭，使 `Vendor.cpp` 只含邏輯而不含裸字串字面值，未來改文案不需掃描邏輯代碼。成交訊息的「四段拼接」設計（而非一個格式字串）允許各段獨立翻譯（例如「花了」→英文「spent」），體現了 i18n 友好的設計考量。`kStockLineSep`/`kStockLineUnit` 常數使庫存對話行的格式在建構（`BuildDialogLines`）和測試期間保持一致，不會因字面值漂移而不匹配。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/vendor/VendorMessages.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorMessages.h) · [← 全檔索引](../files-index.md)
