---
id: "file:tests/ui/test_font_ui_literal_scan.cpp"
type: test
path: tests/ui/test_font_ui_literal_scan.cpp
domain: tests
bucket: ui
loc: 349
classes: []
sources: ["tests/ui/test_font_ui_literal_scan.cpp"]
---
# `test_font_ui_literal_scan.cpp`

> **一句定位**：永久性「無 ? 豆腐」字型閘——直接掃描 `src/` 與 `include/` 的原始碼字串字面及 `docs/content/*.md`，確保每個中文字元都已烘進字型圖集，且僅出現在原始碼字面的字元都已手動烘入 `UiLiteralChars()`。

## 職責

這是本專案字型防豆腐測試中最底層、最廣泛的一層。它解決了「字形掃描測試已全綠，但實際執行時某個零散字串字面（ShowMessage 台詞、商人問候、撿道具台詞）仍出現 ? 」的根本問題。

測試分兩個斷言：

**斷言 (A)**：掃描 `src/` + `include/` 下所有 `.cpp/.h` 的「字串字面」，以及 `docs/content/*.md` 的所有字元，組成「有效圖集」（ASCII ∪ `UiLiteralChars` ∪ docs/content），確認每個大於 U+2010 的碼位都在圖集中。

**斷言 (B)**：找出僅出現在原始碼字面、不在任何 `.md` 的字形，要求它們全都在 `UiLiteralChars()` 中。這正是抓出「敬」字（預告字）的條款——它不在任何 `.md`，若未手動烘入則閘變紅。

測試附帶一個具名固定測試，驗證一批已知曾豆腐的字形（敬刺君含扶毫央櫃牽羊及中文彎引號）現已在 `UiLiteralChars()` 中。

原始碼掃描器 `ExtractLiteralBytes` 是一個 C++ 字面提取的小型狀態機，能正確跳過 `//` 與 `/* */` 注釋及字元字面，並解析 `\xHH`、`\n`、`\uXXXX` 跳脫。

需要建置系統定義 `TEST_SOURCE_DIR`（src/include 的根目錄）與 `TEST_CONTENT_DIR`（docs/content）。

## 關鍵內容（類別 / 函式 / 資料）

- `ExtractLiteralBytes(src)`：C++ 原始碼清洗器，移除注釋、保留字串字面的解碼後位元組，含 `St` 狀態枚舉（Code/Slash/LineComment/BlockComment/BlockStar/Str/StrEsc/Chr/ChrEsc）。
- `DecodeUtf8Into(str, set<int>&)`：UTF-8 → 碼位的自足解碼器，與 `DialogLayout.cpp` 一致。
- `Utf8Len(byte)`：從前導位元組推算 UTF-8 序列長度。
- `ScanSourceLiterals()`：遞迴走訪 `src/` + `include/` 下所有 `.cpp/.h`，收集字串字面碼位。
- `ScanContent()`：走訪 `docs/content/*.md` 收集碼位。
- `UiLiteralCodepoints()`：解碼 `UiLiteralChars()` 回傳的字串，建出確定性字面集合。
- `Interesting(cp)`：過濾 U+2010 以下的碼位（ASCII 或低值符號一律安全）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Font.h`（`CollectCodepoints`、`UiLiteralChars`）、及標準函式庫 `<filesystem>`、`<fstream>` 等。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—

## OO 概念與設計重點

此檔是三層字型防護中最底層的**靜態分析閘**：掃描原始碼而非執行期呼叫，讓即使是極少被走到的字串字面也無法通過漏洞。使用 `<filesystem>` 遞迴遍歷確保即使新增檔案也能被覆蓋。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_font_ui_literal_scan.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_font_ui_literal_scan.cpp) · [← 全檔索引](../files-index.md)
