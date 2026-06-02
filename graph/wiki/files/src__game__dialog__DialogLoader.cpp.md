---
id: file:src/game/dialog/DialogLoader.cpp
type: source
path: src/game/dialog/DialogLoader.cpp
domain: game
bucket: dialog
loc: 354
classes: []
sources: ["src/game/dialog/DialogLoader.cpp"]
---
# `DialogLoader.cpp`

> **一句定位**：章節對白 Markdown 的完整解析器——以手寫掃描逐行解析 NPC 標題、(a–d) 子段、台詞行及 karma/旗標中介資料，輸出 `LoadedChapter` 結構。

## 職責

本檔在 `nccu::dialog` 命名空間內實作 `LoadChapter(path) -> LoadedChapter`，是對白系統唯一讀取磁碟的入口。設計上刻意不引入正規表示式，全程逐位元組手寫掃描，以精準處理全形標點與 CJK 引號等多位元組序列。

**文法層次**：Markdown 章節檔按 `## NPC：<名稱>`（NPC 標題）→ `### (a–d) <標題>`（子段）→ `- "台詞"` / `- "台詞"`（台詞行）→ `> ... // karma ±N` / `> ... Flag_X = true|false`（中介資料引用區塊）的層次解析。

**NPC 標題解析（`ParseNpcName`）**：識別 `## NPC` 前綴，以全形冒號（U+FF1A）為分隔取名稱，並剝除尾端的 `（…）` 全形括注（作者撰寫時的鷹架標記，非角色名的一部分）。

**子段解析（`ParseSubStateHeader`）**：嚴格匹配 `### (a–d)` 格式（字母限 a–d），取標題文字交給 `CleanLabel` 清理：優先以 `「…」` 角括號引號內側文字作為選項標籤；否則剝除尾端的 `（…）` 全形括注後修剪空白。

**台詞行解析（`ParseDialogLine`）**：識別 `- "…"` 格式，支援 ASCII `"` 與 CJK `"…"` 兩種引號對。內文為空則失敗。

**中介資料解析**：`ScanKarma` 從 `>` 引用區塊掃描 `// karma ±N`；`ScanFlag` 掃描 `Flag_X = true|false`。各規則模擬 `//\s*karma\s*([+-]\d+)` 與 `\b(Flag_[A-Za-z0-9_]+)\s*=\s*(true|false)\b`，並實作「取第一個非零 karma」「取第一個非空旗標」的防護——後續同類註記不覆寫。

**收尾穩定排序**：所有 NPC 的 `SubEntry` 陣列以 `std::stable_sort` 按 `subState` 升序排列（a<b<c<d），強制執行模型不變式。

## 關鍵內容（類別 / 函式 / 資料）

- `LoadChapter(const string& path) -> LoadedChapter`：公開 API，逐行解析並回傳完整模型。
- `ParseNpcName(line)` → `string`：解析 `## NPC：<name>` 標題，剝除括注。
- `ParseSubStateHeader(line, letter&, heading&)` → `bool`：解析 `### (a–d)` 子段。
- `CleanLabel(raw)` → `string`：選項標籤清理（`「…」` 優先，否則剝括注）。
- `ParseDialogLine(line, out&)` → `bool`：解析 `- "台詞"` 行，支援兩種引號。
- `ScanKarma(line, value&)` → `bool`：掃描 `// karma ±N` 中介資料。
- `ScanFlag(line, name&, val&)` → `bool`：掃描 `Flag_X = true|false` 中介資料。
- `RStripCr(string&)`：去除 Windows CRLF 尾端 `\r`。
- `kFullWidthColon` / `kLeftCjkQuote` / `kRightCjkQuote` / `kLeftCornerQuote` / `kRightCornerQuote` / `kFullWidthParenL` / `kFullWidthParenR`：多位元組 UTF-8 字節序列常數。

## 相依與在架構中的位置

- **#include（往外）**：`DialogLoader.h`（公開宣告與 `LoadedChapter` / `SubEntry` 型別）；標準庫 `<fstream>` / `<algorithm>` / `<cstdlib>` / `<string>`，無 raylib。
- **被誰使用（往內）**：`DialogSource.cpp`（`DialogRepository::ChapterFor` 呼叫 `LoadChapter`）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：Model 層的資料載入器；僅在第一次需要某章節時由 `DialogRepository` 惰性呼叫，結果快取於 `cache_` 中。

## OO 概念與設計重點

本檔是一個純粹的 **資料解析模組**，不持有任何執行期可變狀態。所有複雜性封裝於匿名命名空間的輔助函式中，公開 API 僅 `LoadChapter` 一個。刻意不用正規表示式以避免多位元組字符的邊界問題，是對「本地化內容處理需要比標準工具更細粒度控制」原則的直接體現。`stable_sort` 保證子段順序的模型不變式在任何內容撰寫順序下皆成立。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogLoader.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogLoader.cpp) · [← 全檔索引](../files-index.md)
