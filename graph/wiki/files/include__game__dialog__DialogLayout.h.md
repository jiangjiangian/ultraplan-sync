---
id: "file:include/game/dialog/DialogLayout.h"
type: header
path: include/game/dialog/DialogLayout.h
domain: game
bucket: dialog
loc: 72
classes: []
sources: ["include/game/dialog/DialogLayout.h"]
---
# `DialogLayout.h`

> **一句定位**：對話框排版的純呈現工具——以 Unicode East Asian Width 格寬計算進行斷行和分頁，確保任何長度的台詞都不溢出對話框，並集中定義所有幾何常數供 View、DialogState 和測試三方一致使用。

## 職責

`DialogLayout.h` 屬 game dialog 層，是一個無 raylib、無輸入、無 World 的純函式標頭，所有函式都是純函式（pure function），可直接單元測試。

核心設計關切：對話框有固定的像素尺寸（`kBoxW=760`、`kBoxH=110`），台詞長度不固定，若不做正確的斷行/分頁，長台詞會溢出或被裁切。`DialogLayout.h` 以 **Unicode East Asian Width（EAW）** 計算每個字元的「視覺格寬」（ASCII/窄字=1 格，CJK 漢字/全形/模糊=2 格），以格數而非位元組數進行斷行，確保「斷行的格數 ≤ `maxCells`」為 renderer 的唯一真實來源。

四個純函式的層次：
1. **`CellWidth(const string& s)`**：計算 UTF-8 字串的視覺格數，組合附加符號計 0 格。
2. **`WrapToCells(const string& s, int maxCells)`**：貪婪斷行，含 ASCII 空白的句子在空白處斷（word wrap），純 CJK 在字元邊界斷，絕不切多位元組序列，字面 `\n` 強制硬斷。
3. **`Paginate(const vector<string>& rows, int rowsPerPage)`**：把已斷行的列切頁，永遠至少回傳一頁（空的），讓呼叫端安全索引 `[0]`。
4. **`LayoutPages(const string& s, int maxCells, int rowsPerPage)`**：便利函式，一次完成斷行再分頁。

幾何常數集中於此：`kBoxCells=80`、`kBoxRowsPerPage=3`、`kBoxX=20`/`kBoxY=320`/`kBoxW=760`/`kBoxH=110`、`kBoxTextX=36`/`kBoxTextY=336`、`kBoxLineH=22`、`kBoxFontSize=16`。設計注解詳細說明了 `kBoxCells=80` 是如何以字型大小/字距反推得出（非螢幕實測）。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `CellWidth(const string& s)` | 計算 UTF-8 字串的 EAW 視覺格數；pure function。 |
| `WrapToCells(const string& s, int maxCells)` | 貪婪斷行；回傳至少一列；絕不截斷多位元組序列。 |
| `Paginate(rows, rowsPerPage)` | 分頁；永遠至少回傳一頁。 |
| `LayoutPages(s, maxCells, rowsPerPage)` | 便利包裝：斷行後分頁。 |
| `kBoxCells = 80` | 對話框每列視覺格數上限（由字型反推）。 |
| `kBoxRowsPerPage = 3` | 每頁列數。 |
| `kBoxX/Y/W/H` | 對話框矩形（20, 320, 760, 110）。 |
| `kBoxTextX/Y` | 文字起始位置（36, 336）。 |
| `kBoxLineH = 22.0f` | 行高。 |
| `kBoxFontSize = 16` | 字級。 |

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<cstddef>`、`<string>`、`<vector>`）。完全不依賴 raylib 或遊戲狀態。
- **被誰使用（往內）**：`src/game/dialog/DialogLayout.cpp`（函式實作）、`src/game/dialog/DialogState.cpp`（分頁計算）、`src/game/dialog/DialogView.cpp`（排版後繪製）、多個 UI 元件（`ChapterCard`/`EndingView`/`InventoryView`/`MessageView`）、大量測試（`test_dialog_layout`/`test_dialog_choice_layout`/`test_chapter_card`/`test_ending_card_render`/`test_inventory_view`/`test_message_view`）。
- **繼承 / 實作 / 體現**：—（純函式工具，無類別）
- **每幀管線 / MVC 角色**：遊戲對話系統的排版層；被 View（繪製）和 Model（`DialogState` 分頁計算）兩層使用，是唯一知道「格寬計算規則」的地方。

## OO 概念與設計重點

`DialogLayout.h` 是**純函式（Pure Function）設計**的典範：沒有任何副作用和全局狀態，相同輸入永遠得到相同輸出，可以任意組合和獨立測試。這讓整個排版邏輯的回歸測試極為簡單——`test_dialog_layout.cpp` 可以直接呼叫 `LayoutPages` 並斷言輸出，無需模擬 World 或 Controller。

「幾何常數集中於此」的設計（而非分散在 View 和 State）確保了「斷行格數 = 對話框可用格數」的不變式在所有呼叫端一致，消除了常數不一致導致的溢出或過短斷行 bug。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogLayout.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogLayout.h) · [← 全檔索引](../files-index.md)
