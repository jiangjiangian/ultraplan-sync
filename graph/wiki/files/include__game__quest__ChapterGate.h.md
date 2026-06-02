---
id: file:include/game/quest/ChapterGate.h
type: header
path: include/game/quest/ChapterGate.h
domain: game
bucket: quest
loc: 36
classes: []
sources: ["include/game/quest/ChapterGate.h"]
---
# `ChapterGate.h`

> **一句定位**：旗標驅動的「章節 ↔ Interlude」推進閘門——`CheckChapterGates` 處理 Ch2→市集、Ch3→市集、市集→returnTo 三種轉場，為可再進入的主軸。

## 職責

`ChapterGate.h` 宣告單一函式 `CheckChapterGates`，是整個遊戲「章節推進」的核心執行者（Ch1→Interlude 仍由 EventWiring 的 `UmbrellaClaimed` 訂閱者處理，因為它同時播下 `returnTo = Chapter2_Midterms`，放在此處會雙重觸發）。

函式以「並列 if」形狀（非 else-if）覆蓋三個獨立轉場：
1. `Chapter2_Midterms + Flag_Ch2Cleared` → `Interlude`（returnTo Ch3）
2. `Chapter3_SportsDay + Flag_Ch3Cleared` → `Interlude`（returnTo Ch4）
3. `Interlude_Market + Flag_LeaveInterlude` → 已記錄的 `returnTo`

任何轉場都先 `dialog.Close()` 掉開啟中的對話，避免殘留的 Active 對話在下一章繼續吃輸入。這個「強制關閉」是防禦性設計，保證章節切換瞬間的對話狀態乾淨。

Ch4→結局由 `EndingGate` 負責，不在此處——職責分離使 `CheckChapterGates` 只處理「章節往返市集」的可重入路徑，`EndingGate` 處理「終焉四結局」的終態轉場。

## 關鍵內容（類別 / 函式 / 資料）

- **`void CheckChapterGates(EventBus& bus, Player& player, SemesterStateMachine& semester, DialogState& dialog)`**（`namespace nccu`）：三條並列 if 轉場；任何轉場都 `dialog.Close()`；Ch4→結局不在此處。

## 相依與在架構中的位置

- **#include（往外）**：—（完全依賴前向宣告）。前向宣告：`Player`（`flags_` 讀取）、`EventBus`（轉場提示 `ShowMessage`）、`SemesterStateMachine`（`Transition`/`InterludeReturnTo`）、`DialogState`（`Close()`）。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（每幀呼叫，在 `LiftChapter1/2Clear` 之後）、`src/game/controller/screens/DialogScreen.cpp`（對話確認後也可觸發）、`src/game/quest/ChapterGate.cpp`（實作）；測試：`test_ch1_quest.cpp`、`test_ch2_quest.cpp`、`test_chapter_spine.cpp`、`test_chapter_transitions.cpp`、`test_interlude_exit.cpp`。
- **繼承 / 實作 / 體現**：—（純自由函式宣告）。
- **每幀管線 / MVC 角色**：Controller 層，每幀在非對話狀態下輪詢，緊鄰 `LiftChapter1/2Clear` 之後、`CheckEndingGates` 之前。

## OO 概念與設計重點

`CheckChapterGates` 採「並列 if」而非 else-if 或繼承的設計，讓未來新增轉場閘門只需在此增加一個 if 分支，符合開閉原則（對擴充開放）。避免泛化（例如 `TransitionRule` 介面）保持了程式碼直讀性，與 `CheckEndingGates` 風格一致，使兩者可以相鄰放置並易於比較。

與 `EndingGate` 的職責分離體現了「閘門按性質分類」：可往返的章節轉場（市集←→章節）與不可逆的結局轉場（終態）有根本性的語意差異，應由不同函式處理。此設計避免了在同一個函式中混合「可重入」與「終態」邏輯，降低了錯誤引入結局的風險。

強制 `dialog.Close()` 是 [RAII](../concepts/oo-raii.md) 思路的程序性類比：轉場時清理對話狀態，確保進入新章節的環境乾淨。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ChapterGate.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterGate.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
