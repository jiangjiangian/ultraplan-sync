---
id: file:src/game/dialog/DialogLayout.cpp
type: source
path: src/game/dialog/DialogLayout.cpp
domain: game
bucket: dialog
loc: 212
classes: [Range]
sources: ["src/game/dialog/DialogLayout.cpp"]
---
# `DialogLayout.cpp`

> **一句定位**：對白方塊的文字排版核心——依 Unicode EAW 計算格寬，正確斷行繁中＋ASCII 混排文字，並分頁供逐行播放。

## 職責

本檔在 `nccu::dialog` 命名空間內實作三個公開函式（`CellWidth`、`WrapToCells`、`Paginate`）及組合函式 `LayoutPages`，是整個對話系統的排版引擎。

**格寬計算**：以匿名命名空間中的 `Utf8Len`（依前導位元組判斷 UTF-8 序列長度）、`DecodeUtf8`（解碼 codepoint）、`IsCombining`（U+0300–U+036F 組合附加符號算 0 格）、`IsWide`（East Asian Width W/F/A 聯集、含 CJK BMP、全形標點等 20 個區間算 2 格）構成。`CellsOf` 整合三者：組合符號 0，寬字 2，其餘 1。`CellWidth(string)` 對整個字串加總格數。

**字串斷行 `WrapToCells`**：支援 ASCII word-wrap（以空白分隔的單字不從中截斷）與 CJK 逐字斷行。維護「待處理單字緩衝」（`word` / `wordW`），遇空白先輸出緩衝再處理空白；遇 CJK / 標點先輸出緩衝再放置該字元。硬斷 `\n`（作者意圖）與軟斷（格寬溢出）以 `softWrapped` 旗標區分：軟斷列頭的偶發空白丟棄，硬斷後的縮排原樣保留（選項標記 `"  "` / `"> "` 的對齊依賴此行為）。

**分頁 `Paginate`**：將 `WrapToCells` 輸出的列陣列每 `rowsPerPage` 列切一頁，至少保留一頁（空輸入仍回傳 `[{}]`）。

`LayoutPages` 組合 `WrapToCells` + `Paginate`，是上層（`DialogState::CurrentPageRows`）實際呼叫的入口。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Range { uint32_t lo, hi; }`（匿名命名空間內）：EAW 寬字區間的 POD 型別。
- `Utf8Len(unsigned char b)` → `size_t`：判斷 UTF-8 序列位元組數（1/2/3/4，無效前導返回 1）。
- `DecodeUtf8(string, i, n)` → `uint32_t`：解碼給定起點、給定長度的 codepoint。
- `IsCombining(uint32_t cp)` → `bool`：U+0300–U+036F 組合附加符號回傳 true。
- `IsWide(uint32_t cp)` → `bool`：查兩組共 20 個 EAW 區間陣列（`kWide` / `kWide2`）。
- `CellsOf(uint32_t cp)` → `int`：0 / 2 / 1。
- `CellWidth(const string& s)` → `int`：公開 API，計算整個字串的視覺格數。
- `WrapToCells(const string& s, int maxCells)` → `vector<string>`：斷行輸出。
- `Paginate(const vector<string>& rows, int rowsPerPage)` → `vector<vector<string>>`：分頁。
- `LayoutPages(const string& s, int maxCells, int rowsPerPage)` → `vector<vector<string>>`：組合入口。

## 相依與在架構中的位置

- **#include（往外）**：`DialogLayout.h`（公開宣告及常數）；匿名命名空間用到標準庫 `<algorithm>` / `<array>` / `<cstdint>`，無 raylib 相依。
- **被誰使用（往內）**：`DialogState.cpp`（`CurrentPageRows` / `CurrentLineHasMorePages`）、`DialogView.cpp`（`WrapToCells` 用於選項標籤換行）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：Model 層的工具模組；`DialogState` 在每次需要顯示文字時呼叫，屬純計算，無副作用。

## OO 概念與設計重點

本檔屬 game / dialog 層的純演算法模組，符合 [DIP](../concepts/arch-dip-renderer.md) 中「engine 不反向相依」的紅線——不引入 raylib、不引入渲染介面。EAW 分區資料以 `static constexpr array<Range,…>` 儲存，屬於查表優先的設計，對本遊戲繁中＋ASCII 混排的實際字集已充分覆蓋並與內容產生工具保持一致。`WrapToCells` 的 lambda（`flushRow` / `placeWord`）使內部邏輯局部化，避免可變狀態外洩到主迴圈外。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogLayout.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogLayout.cpp) · [← 全檔索引](../files-index.md)
