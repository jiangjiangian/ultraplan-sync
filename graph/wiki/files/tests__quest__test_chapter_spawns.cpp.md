---
id: file:tests/quest/test_chapter_spawns.cpp
type: test
path: tests/quest/test_chapter_spawns.cpp
domain: tests
bucket: quest
loc: 180
classes: []
sources: ["tests/quest/test_chapter_spawns.cpp"]
---
# `test_chapter_spawns.cpp`

> **一句定位**：驗證 Ch1 名冊資料與 World 生成行為：Player 不變量（永遠在物件清單最前端）、`RespawnChapterRoster` 抽換機制、往返切換的完整復原，以及重複 respawn 不重複生成。

## 職責

此測試檔是 World 名冊系統的主要整合測試，覆蓋四個面向：

1. **Ch1 資料表**：`ChapterNpcSpawns(Chapter1_AddDrop)` 恰好含 5 個原型（`{victim, suit_senior, bookworm, ta, shop_auntie}`），且逐欄與舊版 `DefaultNpcSpawns()` 相同；幕間市集與四個結局皆為空（加有設計意圖的段落說明）。

2. **World 建構子**：`World("", false)` 生成 Ch1 名冊後 Player 必須在 `Objects().front()`。

3. **`RespawnChapterRoster` 抽換 + Player 不變量**：切換到 `Ending_A`（穩定空狀態）時所有 NPC 消失、Player 身分和前端位置不變；往返切換回 Ch1 後完整復原（含風味 NPC 和人潮）。測試記錄了 `totalCh1`、`ch1Cash==5`，切換後確認 `Objects().size() < totalCh1`（名冊確實被清除）；非名冊物件（玩家 + 道德傘 + 申請書等）繼續存活。

4. **重複 respawn 不洩漏**：對同一狀態重複呼叫兩次 `RespawnChapterRoster` 後物件總數不變，原型計數仍為 5。

匿名 namespace 提供 `RosterNpcIds`、`HasNpc`、`Ch1Archetypes()`、`ArchetypeNpcCount`、`RosterCashCount` 等 helper，使斷言精確到「主線原型」而不受風味 NPC 干擾。

## 關鍵內容（類別 / 函式 / 資料）

- `RosterNpcIds(const World&)`：收集所有非空 `NpcId` 的集合。
- `ArchetypeNpcCount(const World&)`：只計算 5 個原型的數量，排除風味 NPC。
- `RosterCashCount(const World&)`：計算 `CashPickup` 物件數（名冊成員，切換時也會清除）。
- `TEST_CASE("ChapterNpcSpawns：Ch1 是 5 個原型；幕間市集與結局皆為空")`：資料表驗證 + 空名冊狀態確認。
- `TEST_CASE("World 建構子生成 Ch1 名冊，Player 維持在最前端")`。
- `TEST_CASE("RespawnChapterRoster 抽換 NPC 名冊但保住 Player 不變量")`：切換→驗證→往返，最完整的整合測試。
- `TEST_CASE("對同一狀態重複 respawn 不會重複生成或洩漏 NPC")`。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterSpawns.h`、`ChapterQuestItems.h`、`CashPickup.h`、`World.h`、`Player.h`、`GameObject.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層（`World`）的物件生命週期管理，直接對應 mark-then-sweep 架構中的名冊抽換操作。

## OO 概念與設計重點

「Player 不變量」是此測試的核心，body `Objects().front()` 的位置契約確保 `World::GetPlayer()` 的 O(1) 存取永遠正確。`Ending_A` 被選作對照狀態是個刻意的設計決策：它是「穩定的空名冊狀態」，不像其他章節可能未來加入 NPC 而讓測試需要更新。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter_spawns.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter_spawns.cpp) · [← 全檔索引](../files-index.md)
