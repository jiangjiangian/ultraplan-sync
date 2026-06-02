---
id: file:src/game/quest/QuestIndicator.cpp
type: source
path: src/game/quest/QuestIndicator.cpp
domain: game
bucket: quest
loc: 46
classes: []
sources: ["src/game/quest/QuestIndicator.cpp"]
---
# `QuestIndicator.cpp`

> **一句定位**：依章節狀態把 NPC 任務感嘆號「!」的顯示判定分派給各章節規則函式，是 View 層繪製 `!` 前的唯一決策閘門。

## 職責

此檔屬於 game / quest 層，只包含一個公開函式 `QuestIndicatorVisible`，接受 `(npcId, isQuestGiver, SemesterState, Player)` 四個參數，以 switch 按章節路由至各章節的判定函式（`Ch1IndicatorVisible`、`Ch2IndicatorVisible`、`Ch3IndicatorVisible`、`Ch4IndicatorVisible`）。

各章節的路由策略各異且有明確的設計理由：

- **Ch1**：主線為苦主 → 西裝學長 → 苦主三段序列。西裝學長在預設 NPC 名冊中 `isQuestGiver=false`，因此 Ch1 路由「不」與 `isQuestGiver` 做 AND，而是把它往下傳給 `Ch1IndicatorVisible` 判斷，避免主線 NPC 被名冊旗標誤屏蔽。
- **Ch2**：同理，學霸在名冊中 `isQuestGiver=false`，但喚醒後仍需亮起 `!`，故路由不短路、把旗標往下傳給 `Ch2IndicatorVisible`。
- **Ch3**：鏈上 3 個節點是唯一任務給予者；此路由以 `isQuestGiver &&` 短路，再交由 `Ch3IndicatorVisible` 依序點亮鏈上的 `!`。
- **Ch4**：終局 NPC 全員 `isQuestGiver=false`，故路由完全改以 npcId 為鍵（`Ch4IndicatorVisible`），僅對助教亮起 `!` 直至玩家做出選擇。
- **Interlude / Ending / 其他**：`default` 分支直接回傳 `isQuestGiver`（沿用原始旗標）。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestIndicatorVisible(string_view npcId, bool isQuestGiver, SemesterState, const Player&)` — 唯一的公開函式；依章節分派至 `Ch1-4IndicatorVisible`，回傳 `bool`。

## 相依與在架構中的位置

- **#include（往外）**：`QuestIndicator.h`（函式宣告）、`Chapter1Quest.h` / `Chapter2Quest.h` / `Chapter3Quest.h` / `Chapter4Quest.h`（各章節 `Ch*IndicatorVisible` 的宣告）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `DrawQuestGiverIndicators`（`src/ui/world/QuestGiverIndicators.cpp`）在 View 的每幀繪製迴圈呼叫，決定是否為某 NPC 畫 `!`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（`ui/world`）的純查詢呼叫；屬每幀 `RenderWorld → DrawQuestGiverIndicators` 步驟，只讀取 `const Player&`，不改寫任何狀態。

## OO 概念與設計重點

此檔採用 [Strategy](../concepts/pat-strategy.md) 精神（以章節 enum 為 key 的 switch 分派）實現每章節的判定策略可互換，但保持單一呼叫介面。把章節判定邏輯封在各章節標頭宣告的函式中，讓 View 完全不含玩法邏輯，符合 [MVC](../concepts/arch-mvc.md) 的 View 只讀 Model 設計紅線。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/QuestIndicator.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/QuestIndicator.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [MVC](../concepts/arch-mvc.md)
