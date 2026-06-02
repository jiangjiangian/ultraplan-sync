---
id: file:tests/quest/test_chapter_questitems.cpp
type: test
path: tests/quest/test_chapter_questitems.cpp
domain: tests
bucket: quest
loc: 184
classes: []
sources: ["tests/quest/test_chapter_questitems.cpp"]
---
# `test_chapter_questitems.cpp`

> **一句定位**：驗證各章任務物品表結構（Ch2 三張筆記、Ch1 苦主之傘）、`QuestFlagPickup` 的計數式提示與集合完成獎勵，以及 World 中筆記的延後生成語義。

## 職責

此測試檔覆蓋四個相關但不同層次的契約：

1. **任務物品表** `ChapterQuestItems`：Ch2 有 3 筆（`kFlagFoundNote1/2/3`），每筆 `completionKarma==3`、`completionFlags.size()==3`、`countMessages.size()==3`；Ch1 有 1 筆（`kFlagHasVictimUmbrella`），無完成集合；其他狀態為空。

2. **計數式提示**（修正「先撿到第 3 張卻印出最後一頁」問題）：`QuestFlagPickup::OnPickup` 依撿取計數（第 N 次撿到 → 第 N 則訊息）而非筆記身分路由。以反向順序（note3 → note2 → note1）撿取，驗證 `lastMsg` 依次為 "first"/"second"/"third"。

3. **`countMessages` 為空時沿用舊路徑**：單一訊息的 `QuestFlagPickup("撿到申請書")` 行為不變。

4. **集合完成獎勵與延後生成**：前兩張不給 karma，最後一張集滿後一次性 +3；World 在進入 Ch2 時不立即生成筆記，設下 `kFlagBookworm`（叫醒學霸）後才生成 3 張（`MaybeSpawnChapter2Notes` 返回 true，且只觸發一次）；離開 Ch2 後隨名冊清除，申請書（不在名冊中）仍存活。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ChapterQuestItems：Ch2 是 3 張筆記；其餘狀態皆為空")`：完整結構驗證。
- `TEST_CASE("QuestFlagPickup：計數式訊息 — 第 N 次撿到 -> 第 N 則，與撿取順序無關")`：反向撿取 + 換順序的兩組驗證；使用 `ScopedSubscribe` 攔截 `ShowMessage`。
- `TEST_CASE("QuestFlagPickup：countMessages 為空時沿用單一訊息")`。
- `TEST_CASE("QuestFlagPickup：集合完成時完成獎勵 karma 只觸發一次")`：逐張撿取 + 2 參數建構子確認無獎勵。
- `TEST_CASE("World 把 3 張 Ch2 筆記延後到學霸被叫醒後才生成，離開章節隨名冊清除")`：World 整合測試，呼叫 `MaybeSpawnChapter2Notes`、`RespawnChapterRoster`。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`ChapterQuestItems.h`、`Chapter2Quest.h`、`QuestFlagPickup.h`、`World.h`、`Player.h`、`GameObject.h`、`EventBus.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層（`World`、`Player`）及任務物品系統（`QuestFlagPickup`）。

## OO 概念與設計重點

計數式訊息修正是個典型的「順序無關不變量」：無論以什麼順序撿筆記，訊息由玩家持有計數決定而非物件身分，確保敘事提示的連貫性。`ScopedSubscribe`（[RAII](../concepts/oo-raii.md) 訂閱）在測試中自動解除訂閱，避免跨 TEST_CASE 洩漏。[Observer 模式](../concepts/pat-observer.md)（`EventBus`）讓 `QuestFlagPickup::OnPickup` 能廣播 `ShowMessage` 而不直接引用 View。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter_questitems.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter_questitems.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
