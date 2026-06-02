---
id: "file:tests/dialog/test_dialog_layout.cpp"
type: test
path: tests/dialog/test_dialog_layout.cpp
domain: tests
bucket: dialog
loc: 226
classes: [Spy]
sources: ["tests/dialog/test_dialog_layout.cpp"]
---
# `test_dialog_layout.cpp`

> **一句定位**：驗證對話排版邏輯（`CellWidth`、`WrapToCells`、`Paginate`、`LayoutPages`、`DrawDialog` 分頁）的完整性，以及端到端確認所有出貨章節台詞算繪後每列都不超過 `kBoxCells`（28 cell）。

## 職責

本檔是對話排版系統最完整的測試套件，包含 10 個 `TEST_CASE`，從底層 cell 寬度計算到端到端全台詞掃描。

**`CellWidth` 語意**：CJK 算 2 cell、ASCII 算 1、全形符號（`（）`）算 4、空字串算 0。

**`WrapToCells` CJK 換行**：略寬於 `kBoxCells` 的 CJK 行至少產生 2 列，每列 `<= kBoxCells`，且無損（還原後等於原字串）。長至 `kBoxCells*4` 的行產生 ≥ 4 列，每列仍在框內。

**`WrapToCells` ASCII 換行**：只在空白處斷，每詞完整保留（joined 等於原文）；單一詞比預算寬時仍不溢出。

**硬換行與縮排**：含 `\n` 的字串強制在換行處斷，前導縮排（`"  > pick me"`）保留。

**`Paginate` 分頁**：7 列依 3 頁大小切成 `[3, 3, 1]`；空輸入至少回傳 1 頁；`LayoutPages` 空字串也回傳 1 頁。

**`DialogState` 長行分頁**：超過一整頁（`kBoxCells * kBoxRowsPerPage` cell）的行自動分成多頁；第一頁有 `kBoxRowsPerPage` 列且 `CurrentLineHasMorePages() == true`；`Advance` 翻頁不換行；再 `Advance` 才到下一行；最終關閉。

**端到端全台詞掃描**：載入 `chapter1..4.md`、`ending_a..c.md`、`interlude_market.md`、`voice_bible.md` 中所有 NPC 的所有 `SubEntry`，以真正的 `DrawDialog + Spy` 走過每一頁，斷言每個算繪列 `CellWidth(t) <= kBoxCells`；合理性檢查 `linesChecked > 200`。

**`DrawDialog` 分頁限制**：一次只畫 `kBoxRowsPerPage` 列（加可能的 `▼` 提示），不多不少；每個算繪列在框內。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Spy : IRenderer`：`rects` + `texts` 的算繪攔截器。
- `CellWidth(string_view)`：全形 2、半形 1 的 cell 寬度計算。
- `WrapToCells(text, maxCells)`：按 cell 數換行，CJK 任意處斷、ASCII 空白處斷。
- `Paginate(rows, rowsPerPage)`：切分為多頁 vector。
- `LayoutPages(text, cols, rows)`：組合 WrapToCells + Paginate。
- `kBoxCells`、`kBoxRowsPerPage`：對話框的寬度（cell 數）與每頁最大列數。
- `DialogState::CurrentLineHasMorePages()`、`CurrentPageRows()`：分頁導航介面。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogLayout.h`、`include/game/dialog/DialogState.h`、`include/game/dialog/DialogView.h`、`include/game/dialog/DialogLoader.h`、`include/engine/render/IRenderer.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 繼承 `IRenderer`
- **每幀管線 / MVC 角色**：驗證 View 層的排版計算，確保對話框不溢出

## OO 概念與設計重點

端到端全台詞掃描是「品質保證測試」的典範：以 `TEST_CONTENT_DIR` 注入真實素材路徑，在引擎邏輯層（而非截圖）驗證不溢出，對任何 CJK 輸入都保持有效。排版常數（`kBoxCells`）驅動的測試構造方式確保「框寬改變時測試不需修改」，體現了正確的抽象層次。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_layout.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_layout.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-dip-renderer](../concepts/arch-dip-renderer.md)
