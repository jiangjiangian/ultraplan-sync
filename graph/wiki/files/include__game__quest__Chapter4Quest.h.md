---
id: file:include/game/quest/Chapter4Quest.h
type: header
path: include/game/quest/Chapter4Quest.h
domain: game
bucket: quest
loc: 116
classes: []
sources: ["include/game/quest/Chapter4Quest.h"]
---
# `Chapter4Quest.h`

> **一句定位**：Ch4「期末考終焉巔峰」漣漪業力的旗標說明文件與四個終盤鉤子宣告——指示燈閘門、溫柔終盤授傘、全章漣漪業力、與結局前自白插入。

## 職責

本標頭是 Ch4（`Chapter4_Finals`）終盤機制的公開 API。chapter4.md 的業力以項目符號記錄（非區塊引用），故沒有任何業力是解析器套用的——每筆 Ch4 業力皆 path-b（由鉤子函式施加）。

**指示燈閘門**：`Ch4IndicatorVisible` 以 `npcId` 為鍵（非 `isQuestGiver`），使終盤唯一推進主線的助教（Ch4 名冊出貨 `isQuestGiver=false`）亮燈；`Flag_TaFinaleChoiceMade` 設立後熄滅。解決 Ch4「終盤沒有任何 `!` 指示燈」的可發現性問題。

**溫柔終盤授傘**：`TryGrantTaFinaleUmbrella` 在玩家選擇「體諒」（`Flag_ConsoledTA` 由 `ApplyDialogChoice` 設置後）把真傘塞回手中（`Flag_HasTrueUmbrella` + `HasUmbrella`）。這是 Ending A 在 `karma > 80` 下的補充路徑（無需隱藏的體育館真傘）。強硬的「質問」分支永不設 `Flag_ConsoledTA`，故永不授傘。具冪等性（`HasFlag` 守衛）。

**全章漣漪業力**：`TryApplyCh4Ripple` 依五個 NPC 各自的旗標組合落地 Ch4 最終業力加減，每 NPC 各一次性：西裝學長（`HelpedSenior && karma>70`→+10，ch4 崩潰坦白）、學霸（`BookwormRecovered`→+5）、助教（`HelpedTA_Ch1`→+10；`HasProfessorTrap`→-15，兩者獨立鍵）、福利社阿姨（`BoughtCoffeeForAuntie_Ch1`→+3，Ch1 情分兌現）。助教的 +10 與 -15 用分開的鍵，使兩者可同時落地。

**結局前自白**：`TryOpenEndingConfession` 在每個「非對話」幀、`CheckEndingGates` 之前輪詢，為三條缺乏收尾節拍的結局路徑（詛咒 → Ending B、醜傘 → Ending C、地面真傘 → 釋然）插入一段簡短自白對話，延遲閘門觸發直到玩家讀完。以一次性鎖鍵（`kFlagCh4Confessed*`）確保每路徑只播一次。

## 關鍵內容（類別 / 函式 / 資料）

- **`bool Ch4IndicatorVisible(npcId, Player&)`**：終盤指示燈；助教 `!Flag_TaFinaleChoiceMade` 時亮燈。
- **`void TryGrantTaFinaleUmbrella(Player&, npcId, state)`**：溫柔終盤授傘；`Flag_ConsoledTA` 把守；冪等。
- **`void TryApplyCh4Ripple(Player&, npcId, state)`**：五個 NPC 各自的最終漣漪業力；每 NPC 各一次性。
- **`bool TryOpenEndingConfession(Player&, DialogState&, state)`**：結局前自白插入；三路徑一次性；回傳 true 表示本次開啟了自白。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/quest/Flags.h`、`include/game/state/SemesterState.h`；前向宣告 `Player`；`class nccu::DialogState`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（每幀呼叫各鉤子）、`src/game/controller/screens/DialogScreen.cpp`（對話確認後呼叫溫柔終盤授傘）、`src/game/dialog/DialogOpener.cpp`、`src/game/quest/Chapter4Quest.cpp`（實作）；多個測試。
- **繼承 / 實作 / 體現**：—（純自由函式宣告，無類別）。
- **每幀管線 / MVC 角色**：Controller 層。`TryOpenEndingConfession` 在每個非對話幀於 `CheckEndingGates` 之前輪詢；其他鉤子在 E 互動管線呼叫。

## OO 概念與設計重點

Ch4 標頭展示了「多路徑結局的最終漣漪業力精算」設計：五個 NPC 的業力效果各自有獨立的一次性鎖鍵，讓敘事設計者可以精確控制每條旗標路徑的業力貢獻，而不需擔心重複計算。助教 +10 與 -15 用分開的鍵是精準的設計：體諒助教（+10）與持 ProfessorTrap（-15）的業力可同時落地，形成 -5 的淨效果，讓「做了壞事但有情分」的玩家感受到複合道德代價。

`TryOpenEndingConfession` 的設計解決了「結局閘門太突兀」的敘事問題——在結局條件滿足但玩家尚未感受到意義感時，插入一段內心獨白作為緩衝。這種「前置對話 + 延後閘門」的模式在 Ch1（`LiftChapter1Clear`）與 Ch2（`LiftChapter2Clear`）中已用過，Ch4 在結局前再次採用，是全遊戲一致的敘事節奏設計。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/Chapter4Quest.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter4Quest.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md)
