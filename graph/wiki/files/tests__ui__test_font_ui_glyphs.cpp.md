---
id: "file:tests/ui/test_font_ui_glyphs.cpp"
type: test
path: tests/ui/test_font_ui_glyphs.cpp
domain: tests
bucket: ui
loc: 93
classes: []
sources: ["tests/ui/test_font_ui_glyphs.cpp"]
---
# `test_font_ui_glyphs.cpp`

> **一句定位**：在字型圖集層固定已知關鍵字形（U+25BC 下翻提示、結局字卡專字、建築名稱字），防止粗心重構 Font.h 時使它們靜默消失。

## 職責

本測試是字型圖集的**固定白名單閘**，鎖定三類歷史上曾出問題或設計上特殊的字形：

1. **U+25BC（▼）下翻提示**：對話框以此符號提示「按鍵繼續閱讀」。它不在任何 docs/content/*.md，也不是普通 ASCII，只能透過 `UiLiteralChars()` 進入圖集。測試亦確認 U+25B2（▲）刻意不要求，因為 UI 只用下翻。

2. **結局字卡字形**：結局 B 字卡「你成為了你曾經最討厭的那種人」中的「討」（U+8A0E）與「厭」（U+53AD）不在任何內容檔，連同全形引號「」（U+300C / U+300D），都只能透過 `UiLiteralChars()` 進入圖集。

3. **每個建築名稱的字形**：View.cpp 在建築標牌顯示「Inside: 」加上 `CurrentBuildingName()`（取自 `buildings::kAll`）。測試驅動真實的 `kAll` 表並對每個名稱逐字比對，並以具名抽查的方式固定歷史曾回報缺字的 8 個（井/仁/勇/塘/夫/志/泳/雩）。

所有測試均無頭安全（`CollectCodepoints()` 讀檔並用 raylib 碼位解碼器，但不需 GL）。

## 關鍵內容（類別 / 函式 / 資料）

- `Contains(vector<int>, int cp)`：查詢碼位是否在圖集中的輔助函式。
- `CollectCodepoints()` — 被測：回傳 `std::vector<int>`，含 ASCII 32–126 ∪ `UiLiteralChars()` ∪ docs/content 碼位。
- `buildings::kAll` — 被掃描的建築名稱表。
- 固定抽查的 8 個碼位：0x4E95（井）、0x4EC1（仁）、0x52C7（勇）、0x5858（塘）、0x592B（夫）、0x5FD7（志）、0x6CF3（泳）、0x96E9（雩）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Font.h`（`CollectCodepoints`）、`game/world/Buildings.h`（`kAll`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—

## OO 概念與設計重點

純 doctest 單元測試。以**具名固定字形**的方式建立「不可退行的最小字集」，讓任何刪去 `UiLiteralChars()` 中這些字形的重構都立即在此失敗，且訊息可讀（而非僅靠廣泛掃描測試產生的 hex 碼位訊息）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_font_ui_glyphs.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_font_ui_glyphs.cpp) · [← 全檔索引](../files-index.md)
