---
id: file:tests/quest/test_chapter4_roster.cpp
type: test
path: tests/quest/test_chapter4_roster.cpp
domain: tests
bucket: quest
loc: 56
classes: []
sources: ["tests/quest/test_chapter4_roster.cpp"]
---
# `test_chapter4_roster.cpp`

> **一句定位**：驗證 Ch4 名冊正好是 5 個原型 NPC、全部 `isQuestGiver=false`，且每個都能從 `chapter4.md` 解析出開場 (a) 台詞；librarian 在 Ch4 確實不存在。

## 職責

此測試檔是 Ch2/Ch3 roster 測試的 Ch4 對應版本，但有一個設計上的重要差異：Ch4 的名冊完全沒有任務給予者（`isQuestGiver==false` for all），因為結局由閘門條件（`CheckEndingGates`）驅動而非任務指示。

**面向一：名冊結構**。`ChapterNpcSpawns(Chapter4_Finals)` 的大小恰為 5，npcId 集合為 `{victim, suit_senior, bookworm, ta, shop_auntie}`，且每一筆都斷言 `isQuestGiver==false`。

**面向二：對話解析**。5 個原型的 sub-state 0 都非空；`librarian` 在 `Chapter4_Finals` 下解析為空（librarian 只在 Ch2）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ChapterNpcSpawns：Ch4 是 chapter4.md 的 5 個原型")`：集合比對 + 迴圈確認所有 `isQuestGiver==false`。
- `TEST_CASE("DialogSource：5 個原型解析到 chapter4.md")`：迴圈驗證開場台詞 + librarian 空段落。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterSpawns.h`、`DialogSource.h`、`SemesterState.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層的名冊資料表。

## OO 概念與設計重點

Ch4 的 `isQuestGiver=false for all` 設計意圖是：結局由閘門（哨兵旗標輪詢）觸發，不靠玩家主動找任務給予者。此測試確保名冊資料表反映此設計意圖，防止有人誤設某個 Ch4 NPC 的 `isQuestGiver=true` 而讓「!」指示器在錯誤的位置亮起（除了助教，後者由 `Ch4IndicatorVisible` 單獨處理）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter4_roster.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter4_roster.cpp) · [← 全檔索引](../files-index.md)
