---
id: file:include/game/quest/QuestIndicator.h
type: header
path: include/game/quest/QuestIndicator.h
domain: game
bucket: quest
loc: 50
classes: []
sources: ["include/game/quest/QuestIndicator.h"]
---
# `QuestIndicator.h`

> **一句定位**：宣告 `QuestIndicatorVisible` 函式，是「某 NPC 本幀是否顯示任務 `!`」判斷的唯一真實來源，將各章節提示邏輯集中在 game 層，讓 View 維持純渲染。

## 職責

`QuestIndicator.h` 宣告一個查詢函式 `QuestIndicatorVisible(npcId, isQuestGiver, state, player)`，是 View 在每個世界物件繪製時查詢「是否顯示任務驚嘆號 `!`」的唯一入口，確保各章節的顯示規則只有一個定義處，任何改動都不需要觸碰 `ui/View.cpp`。

詳細的章節邏輯由文件說明：Ch1 單一主線（苦主的 `!` 隨「承諾→找傘→歸還」推進後熄滅）；Ch2 圖書館管理員→學霸主線（管理員為鏈頭，學霸的 `isQuestGiver=false` 不套用名冊旗標而套用進度旗標）；Ch3 依序點亮的 A→B→C 物物交換鏈（5 個路人原型在 Ch3 名冊中皆 `isQuestGiver=false` 故保持熄滅）；Ch4 僅助教（直到 `kFlagTaFinaleChoiceMade` 設立）；幕間與結局直接沿用名冊的 `isQuestGiver` 旗標。

「找錯主線 NPC」的轉向提示不在此處，而在各章的 E 互動鉤子（`TryReturnVictimUmbrella` 等），鉤子發出 `ShowMessage` 提醒而非推進進度。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestIndicatorVisible(string_view npcId, bool isQuestGiver, SemesterState state, const Player& player) → bool`：以 `(npcId, state, player, isQuestGiver)` 組合判定本幀是否顯示 `!`；實作在 `QuestIndicator.cpp`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`（章節狀態參數）；前向宣告 `Player`
- **被誰使用（往內）**：`src/game/quest/QuestIndicator.cpp`（實作體）、`src/ui/View.cpp`（每個可見 NPC 的 `!` 繪製判定）、`src/ui/world/QuestGiverIndicators.cpp`（任務給予者指示器繪製）、`tests/quest/test_quest_indicator.cpp`（單元測試）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層的輸入函式——由 View 在每幀繪製世界時呼叫（每個 NPC 一次），結果決定是否在 NPC 頭上畫「!」圖示；邏輯本身屬 game 層（讀取 Player 旗標），使 View 不持有 game 邏輯。

## OO 概念與設計重點

這個設計實現了**關注點分離（Separation of Concerns）**：任務提示的「是否顯示」邏輯從 View 抽出到 game 層，View 的 `DrawQuestIndicator` 只需知道「true/false」，而不需要嵌入任何進度旗標查詢。結合 [MVC](../concepts/arch-mvc.md) 架構，這使得 `ui/View.cpp` 對任務進度完全無知，所有業務決策都在 game 層做出。函式簽章傳入 `const Player&` 而非 `World&`，也是最小依賴的好設計。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/QuestIndicator.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestIndicator.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
