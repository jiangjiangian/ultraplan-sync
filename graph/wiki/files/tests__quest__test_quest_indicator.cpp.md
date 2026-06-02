---
id: file:tests/quest/test_quest_indicator.cpp
type: test
path: tests/quest/test_quest_indicator.cpp
domain: tests
bucket: quest
loc: 173
classes: []
sources: ["tests/quest/test_quest_indicator.cpp"]
---
# `test_quest_indicator.cpp`

> **一句定位**：驗證 `QuestIndicatorVisible` 與各章專屬子函式（`Ch1/Ch2/Ch4IndicatorVisible`）的「!」點亮邏輯：依章節、npcId、旗標組合決定是否顯示任務指示器，且即使名冊 `isQuestGiver=false` 也能正確點亮特定 NPC。

## 職責

此測試檔是任務「!」視覺指示系統的完整測試，覆蓋四個章節的指示器序列。

**Ch4**：助教是唯一點亮的 NPC（即使 `isQuestGiver=false`），直到 `kFlagTaFinaleChoiceMade` 設下後熄滅；其餘原型始終熄滅。`QuestIndicatorVisible` 透過 npcId 為鍵的路由確保此行為與名冊旗標無關。

**Ch3**：`vendor_sausage_a`（鏈頭 A）從進場就點亮；B/C 兩環在輪到它們之前熄滅；原型 NPC（`isQuestGiver=false`）即使與進行中的鏈同處一章也維持熄滅（`&&` 條件保護）。

**Ch1**：三步「!」序列——
1. 承諾前：只有苦主（isQuestGiver=true）點亮；學長暗。
2. 設 `kFlagPromisedVictim` 後：學長（isQuestGiver=false）點亮，苦主熄滅。
3. 設 `kFlagSuitSeniorChoiceMade` 後：「!」回到苦主，學長再度熄滅。
4. 設 `kFlagHasTrueUmbrella` 後：所有主線「!」熄滅。
還有 `some_future_giver`（isQuestGiver=true）會透過 fall-through 點亮的正向 case。

**Ch2**：管理員（鏈頭）→ 學霸（含自販機中繼）→ 學霸貫穿收集筆記 → 換回後全滅的完整序列。確認 `kNpcCh2Vendor`（自販機）在有 EnergyDrink 後熄滅、叫醒後永遠熄滅。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::QuestIndicatorVisible(npcId, isQuestGiver, state, player)`：統一入口，依章節分派。
- `nccu::Ch1IndicatorVisible(npcId, isQuestGiver, player)`：三步序列邏輯。
- `nccu::Ch2IndicatorVisible(npcId, isQuestGiver, player)`：管理員→學霸→自販機鏈。
- `nccu::Ch4IndicatorVisible(npcId, player)`：僅助教，直到結算定案。
- `TEST_CASE("Ch4IndicatorVisible：助教是結局的「!」，直到結算選擇定案")`。
- `TEST_CASE("QuestIndicatorVisible Ch4：助教不論名冊旗標都會點亮")`：isQuestGiver=false 但仍亮。
- `TEST_CASE("QuestIndicatorVisible Ch3：鏈頭跑圈前點亮，原型維持熄滅")`。
- `TEST_CASE("Ch1 的「!」序列 苦主 -> 西裝學長 -> 苦主")`：完整三步 + 完成熄滅 + 學長的 isQuestGiver=false 特例。
- `TEST_CASE("QuestIndicatorVisible Ch2 的「!」序列 管理員 → 學霸 →（自販機）→ 學霸")`。
- `TEST_CASE("Ch2 非主線 NPC 維持其 isQuestGiver 旗標")`。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`QuestIndicator.h`、`Chapter1Quest.h`、`Chapter2Quest.h`（`kNpcCh2Vendor`）、`Chapter3Quest.h`、`Chapter4Quest.h`、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 View 層呼叫的 `QuestIndicatorVisible`（View 每幀 `Draw` 時決定是否繪製「!」）。

## OO 概念與設計重點

`QuestIndicatorVisible` 是 View 的「單一真實來源」設計：View 不直接讀 `isQuestGiver` 旗標，而是透過這個函式，確保所有章節特殊規則都集中在一處。Ch1 學長與 Ch4 助教的「以 npcId 為鍵凌駕 isQuestGiver」是一個刻意的特例處理，此測試精確鎖住這兩個例外。[Strategy 模式](../concepts/pat-strategy.md)體現在按章節分派到各章子函式的結構上。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_quest_indicator.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_quest_indicator.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md)
