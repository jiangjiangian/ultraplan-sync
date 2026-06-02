---
id: "file:tests/ui/test_font_ui_glyph_scan.cpp"
type: test
path: tests/ui/test_font_ui_glyph_scan.cpp
domain: tests
bucket: ui
loc: 155
classes: []
sources: ["tests/ui/test_font_ui_glyph_scan.cpp"]
---
# `test_font_ui_glyph_scan.cpp`

> **一句定位**：掃描所有由程式動態組出的 UI 字串，確認每個字元皆已烘進中文字型圖集，防止日後新增字串時悄悄出現「?」豆腐。

## 職責

本測試是 UI 字型涵蓋的**動態掃描閘**，與 `test_font_ui_glyphs.cpp` 的固定字形白名單互補。後者固定歷史已知的特定字形；本檔則掃描實際由 API 組出的 UI 字串，只要任一字不在圖集就失敗。

測試使用輔助函式 `RequireAllCovered(atlas, str, label)`：用 raylib 自己的 `LoadCodepoints` 把 UTF-8 字串解碼成碼位，再對照 `CollectCodepoints()` 回傳的完整圖集。無頭安全（不需 GL 環境）。

掃描範圍涵蓋：
1. 結局卡所有字串（`EndingCardStrings()`，驗證 > 10 項）。
2. 章節書擋大卡字串（`ChapterCardStrings()`，驗證 ≥ 8 項）。
3. 遊戲說明扁平清單（`kGameHelpLines` + `kGameHelpClosing`）。
4. 分頁後的遊戲說明（`kGameHelpPages`，驗證恰好兩頁）。
5. View.cpp 的 HUD/選單/物品欄字面（手工列舉的 `ViewLiterals()`，含「金幣: %d 元」、「M 選單」、「物品欄」等）。
6. HUD 目標文字（`QuestObjectiveStrings()`，驗證 ≥ 9 項）。
7. 道具表名稱與說明（`CatalogStrings()`，驗證 > 10 項）。
8. 商人提示訊息片段（`kInsufficientFunds`、`kPurchasedPrefix`、`kSoldOut` 等 8 個）。

## 關鍵內容（類別 / 函式 / 資料）

- `RequireAllCovered(atlas, s, whatFor)`：對每個從 s 解出的非零碼位斷言存在於 atlas；用 doctest `INFO` / `CHECK` 報告缺字。
- `ViewLiterals()`：靜態函式，手工列出 View.cpp 的中文字面，作為自動推導掃描的補充。
- `CollectCodepoints()`：被測（間接）——建立烘進圖集的字形集合。
- `EndingCardStrings()` / `ChapterCardStrings()` / `kGameHelpLines` / `kGameHelpPages` 等 — 被掃描的 API。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Font.h`（`CollectCodepoints`）、`ui/EndingView.h`、`ui/ChapterCard.h`、`ui/GameHelp.h`、`game/quest/ItemCatalog.h`、`game/quest/QuestObjective.h`、`game/vendor/VendorMessages.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純字型涵蓋閘，不接觸 Update/Draw 管線）

## OO 概念與設計重點

純 doctest 單元測試，屬**字型涵蓋閘**設計模式：用 API 驅動式掃描取代人工白名單，使新增的 UI 字串自動觸發閘。與 `test_font_ui_literal_scan.cpp` 搭配形成雙層防禦：本檔保護動態組字，後者保護原始碼字面。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_font_ui_glyph_scan.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_font_ui_glyph_scan.cpp) · [← 全檔索引](../files-index.md)
