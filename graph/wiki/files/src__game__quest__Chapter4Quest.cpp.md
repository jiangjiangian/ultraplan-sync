---
id: file:src/game/quest/Chapter4Quest.cpp
type: source
path: src/game/quest/Chapter4Quest.cpp
domain: game
bucket: quest
loc: 131
classes: []
sources: ["src/game/quest/Chapter4Quest.cpp"]
---
# `Chapter4Quest.cpp`

> **一句定位**：第四章（期末考）終局任務邏輯——助教 `!` 可見性、溫柔終局授予真傘、各 NPC 漣漪業力，以及依結局優先序觸發的自白對白。

## 職責

本檔實作第四章（`Chapter4_Finals`）的四個函式，覆蓋終局機制的各個面向。

**`Ch4IndicatorVisible`**：助教是終局唯一亮 `!` 的對象（在 `kFlagTaFinaleChoiceMade` 設下之前持續亮；`kDialogExitLabel` 退出不設此旗標，故退出後 `!` 仍亮）。其餘所有第四章 NPC 不亮，防止漣漪風味原型造成 `!` 雜訊。

**`TryGrantTaFinaleUmbrella`**：溫柔終局（`kFlagConsoledTA`）的真傘授予。持有 `kFlagConsoledTA` 且尚未持真傘時，呼叫 `SetHeldUmbrella(True)` + `SetFlag(kFlagHasTrueUmbrella)`。本函式本身不「強制」Ending A，只確保「體諒＋高 karma」在沒有隱藏 Ch4 傘的情況下仍能達成持傘條件。

**`TryApplyCh4Ripple`**：五路 NPC 漣漪業力，各用獨立一次性旗標：
- `suit_senior`：(b) 路線（HelpedSenior && karma>70）→ karma +10。
- `bookworm`：BookwormRecovered → karma +5。
- `shop_auntie`：BoughtCoffeeForAuntie → karma +3（直接情報路線）。
- `ta`：HelpedTA_Ch1 → karma +10（一次性）；HasProfessorTrap → karma -15（另一個一次性）；兩者可同時成立，淨得 +10-15 = -5。

**`TryOpenEndingConfession`**：按結局優先序（詛咒 B > 醜傘 C > 真傘 A）觸發自白對白（各三行），以對應的一次性旗標確保每段只播一次：
- 詛咒自白：`kFlagTookCursedUmbrella` + `!kFlagCh4ConfessedCursed`
- 醜傘自白：`kFlagBoughtUglyUmbrella` + `!kFlagCh4ConfessedUgly`
- 真傘自白：`kFlagHasTrueUmbrella` + `!kFlagTaFinaleChoiceMade` + `!kFlagCh4ConfessedTrue`（溫柔終局播自己的後續台詞，故閘控避免重複）

## 關鍵內容（類別 / 函式 / 資料）

- `Ch4IndicatorVisible(npcId, Player&)` → `bool`：只助教亮，確認後熄滅。
- `TryGrantTaFinaleUmbrella(Player&, npcId, state)`：溫柔終局真傘授予（冪等）。
- `TryApplyCh4Ripple(Player&, npcId, state)`：五路漣漪業力，獨立一次性旗標。
- `TryOpenEndingConfession(Player&, DialogState&, state)` → `bool`：三段自白優先序觸發。
- `kFlagConsoledTA` / `kFlagTaFinaleChoiceMade` / `kFlagCh4ConfessedCursed/Ugly/True`：終局關鍵旗標。
- `kFlagCh4RippledSenior/Bookworm/Auntie/TAHelped/ProfTrap`：各漣漪一次性旗標。

## 相依與在架構中的位置

- **#include（往外）**：`Chapter4Quest.h`（旗標常數）、`Player.h`（所有旗標/業力/持傘操作）、`DialogState.h`（`dialog.Open` / `dialog.Active()`）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 互動鉤子與輪詢呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：`TryOpenEndingConfession` 每非對話幀輪詢；`TryApplyCh4Ripple` 在各 NPC 互動時觸發；`Ch4IndicatorVisible` 在渲染前查詢。

## OO 概念與設計重點

本檔展示了「旗標驅動的多路優先序」設計：`TryOpenEndingConfession` 的優先序（詛咒 > 醜傘 > 真傘）與 `CheckEndingGates` 的結局優先序一致，確保玩家讀到的自白正是最終觸發的那個結局。助教漣漪「HelpedTA +10 與 ProfTrap -15 各用獨立一次性鍵」是細緻的業力設計：兩個效果互不阻擋，使「幫過助教且拿過教授傘」的玩家能精確收到淨 -5 的道德帳單。[Observer](../concepts/pat-observer.md) 體現於對白事件驅動 HUD 顯示。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/Chapter4Quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter4Quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
