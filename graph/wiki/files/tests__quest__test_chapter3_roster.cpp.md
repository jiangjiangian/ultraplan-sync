---
id: file:tests/quest/test_chapter3_roster.cpp
type: test
path: tests/quest/test_chapter3_roster.cpp
domain: tests
bucket: quest
loc: 68
classes: []
sources: ["tests/quest/test_chapter3_roster.cpp"]
---
# `test_chapter3_roster.cpp`

> **一句定位**：驗證 Ch3 名冊包含正好 8 個 NPC（5 個原型 + 3 個物物交換鏈節點），且 3 個鏈節點全部標為任務給予者並能解析到 `chapter3.md`。

## 職責

此測試檔鎖定 Ch3（`Chapter3_SportsDay`）名冊與對話解析的契約，平行對照 test_chapter2_roster 的結構但針對運動會的特殊設計。

**面向一：名冊完整性**。8 個 NPC 的 npcId 集合為 `{victim, suit_senior, bookworm, ta, shop_auntie, vendor_sausage_a, loudspeaker_b, senior_c}`，其中只有 3 個物物交換鏈節點（`vendor_sausage_a`、`loudspeaker_b`、`senior_c`）的 `isQuestGiver==true`；5 個原型的 `isQuestGiver==false`（在 Ch3 它們是漣漪內容、不是主線推進者）。

**面向二：對話解析**。3 個鏈節點的 sub-state 0 都非空（開場 (a) 有台詞）。5 個原型在 Ch3 仍可解析（`suit_senior` 作為代表）。Ch3 專屬 npcId（`senior_c`）在 `Ending_A` 下退化為空。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ChapterNpcSpawns：Ch3 是 chapter3.md 的 8 個 NPC")`：集合比對 + 精確確認 `questGivers` 集合為三個鏈節點。
- `TEST_CASE("DialogSource：3 個 Ch3 交換鏈 npcId 解析到 chapter3.md")`：迴圈驗證三個 npcId 都可解析並有開場台詞；`suit_senior` 確認原型仍可解析；結局退化為空。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterSpawns.h`、`DialogSource.h`、`SemesterState.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層的章節名冊資料表。

## OO 概念與設計重點

物物交換鏈（A→B→C）是 Ch3 的核心任務結構，`isQuestGiver` 旗標精確標示哪些 NPC 是主線推進者，`QuestIndicatorVisible` 則依此點亮「!」。此測試固定了這條連結的資料端，確保日後加入或移除鏈節點時，名冊定義與視覺指示同步更新。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter3_roster.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter3_roster.cpp) · [← 全檔索引](../files-index.md)
