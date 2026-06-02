---
id: file:tests/quest/test_chapter2_roster.cpp
type: test
path: tests/quest/test_chapter2_roster.cpp
domain: tests
bucket: quest
loc: 66
classes: []
sources: ["tests/quest/test_chapter2_roster.cpp"]
---
# `test_chapter2_roster.cpp`

> **一句定位**：驗證 Ch2 名冊包含正好 6 個 NPC（含新角色 librarian 且標為任務給予者），以及 librarian 能從 `chapter2.md` 解析到純資訊對話段落（無 karma 無旗標）。

## 職責

此測試檔鎖定 Ch2（`Chapter2_Midterms`）名冊資料與對話解析的契約，分兩個面向驗證：

**面向一：名冊完整性**。`ChapterNpcSpawns(Chapter2_Midterms)` 必須回傳 6 個 NPC，其 npcId 集合恰好為 `{victim, suit_senior, bookworm, ta, shop_auntie, librarian}`，且只有 `librarian` 的 `isQuestGiver==true`（圖書館管理員是 Ch2 推進主線的關鍵 NPC）。

**面向二：對話解析**。透過 `nccu::dialog::Entries("librarian", Chapter2_Midterms)` 確認 librarian 的段落非空，sub-state 0（開場 (a)）有台詞，且所有 sub-entry 都 `karmaDelta==0` 且 `setsFlag` 為空（純資訊節點）。最後以 `Entries("librarian", Ending_A)` 驗證結局狀態下安全退化為空（librarian 只在 Ch2 存在）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ChapterNpcSpawns：Ch2 是 chapter2.md 的 6 個 NPC")`：集合比對 + `librarian->isQuestGiver` 斷言。
- `TEST_CASE("DialogSource：librarian 解析到 chapter2.md 內容")`：sub-state 0 有台詞、全 entry 無 karma 無旗標、結局狀態退化為空。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterSpawns.h`（`ChapterNpcSpawns`）、`DialogSource.h`（`SetContentDir`、`Entries`）、`SemesterState.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層（名冊資料表）與 `DialogSource` 解析管線的契約。

## OO 概念與設計重點

此測試體現「資料表守門」模式：名冊是宣告式資料（純結構體陣列），測試用集合比對確認每個章節的 NPC 集合不因重構而靜默改變。`isQuestGiver` 旗標與 `QuestIndicatorVisible` 的連接使名冊定義具有視覺後果，本測試正是鎖住這條連結的最前端。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter2_roster.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter2_roster.cpp) · [← 全檔索引](../files-index.md)
