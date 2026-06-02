---
id: file:include/game/quest/QuestObjective.h
type: header
path: include/game/quest/QuestObjective.h
domain: game
bucket: quest
loc: 112
classes: []
sources: ["include/game/quest/QuestObjective.h"]
---
# `QuestObjective.h`

> **一句定位**：定義 HUD 頂端任務指引字串常數與 `CurrentObjective` inline 函式，是「接下來該做什麼」一行文字的唯一定義處，同時提供 `QuestObjectiveStrings()` 供字形覆蓋掃描驗證。

## 職責

`QuestObjective.h` 以 `namespace nccu::objective` 下的 `inline constexpr string_view` 常數定義每一條可能顯示的任務指引文字，並提供兩個 inline 函式：`CurrentObjective(state, player)` 依章節狀態與關鍵旗標選出當前該顯示的那一行，以及 `QuestObjectiveStrings()` 列舉所有可能文字供字形覆蓋掃描。

Ch1 有三段依旗標推進的指引：`kCh1MeetVictim`（尚未承諾）→ `kCh1FindUmbrella`（已承諾但未撿傘，以 `kFlagHasVictimUmbrella` 為門檻）→ `kCh1ReturnUmbrella`（傘在手，等待歸還）。幕間市集顯示 `kInterludeMarket`；Ch2 三段（找管理員/學霸 `kCh2FindBookworm`、找三頁筆記 `kCh2FindNotes` 依 `Chapter2NotesComplete(player)` 判定、還筆記 `kCh2ReturnNotes`）；Ch3 單段 `kCh3SportsDay`；Ch4 單段 `kCh4Finals`；四種結局回傳空字串（此時 View 已用結局卡片取代整個世界）。

設計意圖是「有限對照」而非對所有旗標組合反應式判讀：每個章節只有最關鍵的一個或少數幾個擋路旗標決定分段，避免 UI 顯示「爆炸式」旗標組合的複雜性。將常數抽成具名 `string_view` 有兩個目的：(a) 讓 `CurrentObjective` 直接拼接；(b) 讓 `QuestObjectiveStrings()` 列舉全部文字，確保所有字形預先烘進字型（曾有缺字問題）。

## 關鍵內容（類別 / 函式 / 資料）

- **`namespace nccu::objective` 中的常數**：`kCh1MeetVictim`、`kCh1FindUmbrella`、`kCh1ReturnUmbrella`、`kInterludeMarket`、`kCh2FindBookworm`、`kCh2FindNotes`、`kCh2ReturnNotes`、`kCh3SportsDay`、`kCh4Finals`（9 個 `inline constexpr string_view`）。
- `CurrentObjective(SemesterState state, const Player& player) → string`：inline，以 switch-on-state 加旗標判斷選出當前指引；結局狀態回傳空字串。
- `QuestObjectiveStrings() → vector<string>`：inline，列舉所有 9 條指引文字，供字形覆蓋掃描。

## 相依與在架構中的位置

- **#include（往外）**：`Player.h`（`HasFlag`、`Chapter2NotesComplete`）、`Chapter1Quest.h`（`kFlagPromisedVictim`、`kFlagHasVictimUmbrella` 使用）、`Chapter2Quest.h`（`Chapter2NotesComplete`）、`SemesterState.h`
- **被誰使用（往內）**：`src/ui/View.cpp`（HUD 頂端指引文字的繪製）、`src/ui/hud/ObjectiveBar.cpp`（任務目標列）、`tests/ui/test_font_ui_glyph_scan.cpp`（字形覆蓋掃描）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層的資料查詢——每幀由 View 呼叫一次 `CurrentObjective` 以決定 HUD 頂端顯示什麼；邏輯屬 game 層（讀取 Player 旗標），View 只獲得字串。

## OO 概念與設計重點

`QuestObjectiveStrings()` 是一種**靜態驗證輔助**：它使測試能在不實際運行遊戲的情況下，掃描每一條指引文字的字形覆蓋情況，確保字型預載完整。「有限對照」的設計選擇（每章只追蹤最關鍵的擋路旗標）體現了 UI 簡化原則：玩家只需知道「下一步」，而非完整的旗標狀態機。章節常數以具名 `string_view` 而非裸字串字面值儲存，讓 `CurrentObjective` 和 `QuestObjectiveStrings()` 共享同一文字源，消除潛在的漂移（若任一處更新文字而另一處未同步）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/QuestObjective.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestObjective.h) · [← 全檔索引](../files-index.md)
