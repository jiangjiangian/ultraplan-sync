---
id: "file:include/game/dialog/DialogLoader.h"
type: header
path: include/game/dialog/DialogLoader.h
domain: game
bucket: dialog
loc: 51
classes: [SubEntry, LoadedChapter]
sources: ["include/game/dialog/DialogLoader.h"]
---
# `DialogLoader.h`

> **一句定位**：章節對白 Markdown 的解析資料模型——定義 `SubEntry`（子段）和 `LoadedChapter`（NPC 名→子段集合的 map），以及 `LoadChapter` 的解析進入點。

## 職責

`DialogLoader.h` 屬 game dialog 層，定義了對白 Markdown 檔案解析後的記憶體資料模型，以及 `LoadChapter` 解析函式。它是把「作者撰寫的 `docs/content/<chapter>.md`」轉換為「可被 `DialogOpener` 路由的結構化資料」的邊界。

**`SubEntry`**：單一 `### (x) ...` 子段的解析結果，包含：
- `subState`：子段字母 `a`→0, `b`→1, `c`→2, `d`→3（`a` 為開場）
- `lines`：此子段的對白行向量
- `choiceLabel`：選單標籤（`…` 引用段優先；否則取 `(x)` 後剝除尾端 `（…）` 的標題文字；`""` 表示非選項）
- `karmaDelta`：`> // karma ±N` 引言中第一個非零值（0 表示無）
- `setsFlag`/`flagValue`：`> Flag_X = true|false` 引言中的第一個旗標設定（`""` 表示無）

**`LoadedChapter`**：一整份章節 Markdown 的解析結果。以 `std::map<std::string, std::vector<SubEntry>>` 儲存，key 為中文 NPC 名（例如 `"西裝學長"`），value 為依 `subState` 遞增排序的子段集合。

**`LoadChapter(const string& path)`**：解析進入點，回傳 `LoadedChapter`；檔案無法開啟時回傳空 `LoadedChapter`，不丟例外（no-throw 契約）。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `struct SubEntry` | 一個子段的完整資料：`subState`（int）、`lines`（`vector<string>`）、`choiceLabel`（string）、`karmaDelta`（int）、`setsFlag`（string）、`flagValue`（bool）。 |
| `struct LoadedChapter` | 一份章節的 NPC 對白 map：`map<string, vector<SubEntry>> npcs`。 |
| `LoadChapter(const string& path)` → `LoadedChapter` | 解析指定 Markdown 檔；no-throw，缺檔時回傳空結果。 |

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<map>`、`<string>`、`<vector>`）。不引入任何遊戲狀態或 raylib。
- **被誰使用（往內）**：`include/game/dialog/DialogRepository.h`（快取 `LoadedChapter`）、`include/game/dialog/DialogSource.h`（透過 `DialogRepository` 使用）；`src/game/dialog/DialogLoader.cpp`（`LoadChapter` 實作）；`tests/dialog/test_dialog_layout.cpp`/`test_dialog_loader.cpp`、`tests/quest/test_loadchapter_chapter1.cpp`（測試解析結果）。
- **繼承 / 實作 / 體現**：—（純資料結構 + 自由函式）
- **每幀管線 / MVC 角色**：遊戲對話資料層（不參與每幀管線）；在 `DialogRepository::ChapterFor()` 中按需（lazy）載入，結果快取在 Repository 中。

## OO 概念與設計重點

`LoadedChapter` 和 `SubEntry` 是典型的**數據傳輸物件（DTO）**：純資料，無行為，無副作用，做為 Markdown 解析結果與對話路由層之間的邊界。`std::map` 的 key 為中文 NPC 名，反映了「作者以中文命名的慣例」直接映射到資料模型，保持了人類可讀的一致性。

`LoadChapter` 的 no-throw 契約與「回傳 sentinel」（空 `LoadedChapter`）模式，與 `ImageDecoder::LoadRgba8Image` 的設計一致——整個引擎在資源載入失敗時以「靜默退化」而非「拋出例外」為首選。這讓測試環境和無 content 目錄的情況都能乾淨退化，無需 try-catch。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogLoader.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogLoader.h) · [← 全檔索引](../files-index.md)
