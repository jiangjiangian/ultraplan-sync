---
id: "file:tests/world/test_world_chapter_roster.cpp"
type: test
path: tests/world/test_world_chapter_roster.cpp
domain: tests
bucket: world
loc: 176
classes: []
sources: ["tests/world/test_world_chapter_roster.cpp"]
---
# `test_world_chapter_roster.cpp`

> **一句定位**：驗證章節 roster 換班的不變量——World ctor 登記 Ch1 NPC 供換班掃除、完整主線無 NPC 外洩、Player 不變量、Ch3 人群生成與廣場定位。

## 職責

本測試確保 `World::RespawnChapterRoster(SemesterState)` 正確管理 NPC 的生存週期，防止前一章節的 NPC 留到下一章節（外洩）。

**Ctor 登記測試**：World ctor 建立後，5 個 Ch1 NPC（victim / suit_senior / bookworm / ta / shop_auntie）都在物件清單；`RespawnChapterRoster(Interlude_Market)` 後全部消失；任務給予者（`IsQuestGiver()`）計數降為 0。

**每個 Ch1 NPC 都在 roster**：執行 `RespawnChapterRoster(Ending_A)` 後任務給予者計數為 0，間接確認所有 Ch1 NPC 都被 roster 追蹤（而非有些被遺漏）。

**完整主線無外洩**：走過 Interlude → Ch2 → Interlude → Ch3 → Interlude → Ch4 → Ending_A 的完整路徑，每一步後的物件清單 NPC id 集合必須與 `ChapterNpcSpawns(state)` 完全相符（攤販跳過，環境學生無 id 不計）。

**Player 不變量**：任何 `RespawnChapterRoster` 路徑（含各結局）後，`GetPlayer()` 指標不變、`Objects().front().get()` 仍指向同一個 Player。

**Ch3 廣場定位**：物物交換鏈三個 NPC（vendor_sausage_a / loudspeaker_b / senior_c）距羅馬廣場中心（1088,960）< 220px。

**Ch3 操場人群**：座標落在操場範圍（x∈[1384,2005]，y∈[541,940]）的物件 ≥ 15 個（5 名跑者 + 10 名閒置者）。

## 關鍵內容（類別 / 函式 / 資料）

- `HasNpcId(World&, const char*)` — 輔助：遍歷物件清單尋找指定 `NpcId()`。
- `QuestGiverCount(World&)` — 輔助：計算 `IsQuestGiver()` 為 true 的 NPC 數量（dynamic_cast<const NPC*>）。
- `ChapterNpcIdSet(SemesterState)` — 輔助：從 `ChapterNpcSpawns(s)` 建出 id 集合。
- `World::RespawnChapterRoster(SemesterState)` — 被測主要函式。
- `World::GetPlayer()` / `World::Objects()` — 被測觀察目標。
- `nccu::ChapterNpcSpawns(SemesterState)` — 被用來驗證「應生成」的 NPC 集合。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/World.h`（受測主體）、`game/quest/ChapterSpawns.h`、`engine/core/GameObject.h`、`game/entities/NPC.h`、`game/entities/Player.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試，驗證 World 的 Spawn/Sweep 生命週期管理（[mark-then-sweep](../concepts/arch-mvc.md) 的章節邊界）。

## OO 概念與設計重點

「完整主線無外洩」測試是最強的不變量，等同每章都做一次 `ChapterSpawns` 的快照比對，保證 roster 換班的邊界正確。Player 不變量測試體現了「Model 的穩定入口」——玩家在整個遊戲中是同一個物件，不被 Sweep 銷毀。Ch3 廣場定位測試是「地理約束」：把座標常數化為可自動驗證的斷言。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/world/test_world_chapter_roster.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/world/test_world_chapter_roster.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
