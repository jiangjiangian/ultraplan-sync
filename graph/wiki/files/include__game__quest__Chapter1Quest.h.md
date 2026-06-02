---
id: file:include/game/quest/Chapter1Quest.h
type: header
path: include/game/quest/Chapter1Quest.h
domain: game
bucket: quest
loc: 163
classes: []
sources: ["include/game/quest/Chapter1Quest.h"]
---
# `Chapter1Quest.h`

> **一句定位**：Ch1「加退選之亂」任務的旗標說明文件與五個互動鉤子的宣告——歸還苦主的傘、交還助教申請書、購買醜綠傘、循序指示燈閘門、與延後章節結束輪詢。

## 職責

本標頭是 Ch1（`Chapter1_AddDrop`）任務鏈的完整公開 API 宣告，所有鉤子皆由 `GameController` 在適當時機呼叫。實際旗標字串常數住在 `quest/Flags.h`；本標頭說明 Ch1 如何「使用」它們。

**核心任務鏈（歸還苦主的傘）**：`TryReturnVictimUmbrella` 有四個分支：已完成（no-op）、未承諾（no-op）、已承諾但無傘（`ShowMessage` 提醒）、授予（清除 `Flag_HasVictimUmbrella` + 設 `Flag_HasTrueUmbrella` + `SetHasUmbrella(true)`）。刻意「不」在授予時發布 `UmbrellaClaimed`，而是延後到 `LiftChapter1Clear` 待 (d) 重逢對話關閉後再發。

**延後章節結束**：`LiftChapter1Clear` 以 `kFlagClearChapter1` 確保每幀輪詢卻恰好觸發一次；發布順序 `ShowMessage` → `UmbrellaClaimed("TrueUmbrella")` 觸發 EventWiring 的 Ch1→Interlude 轉場。

**助教申請書支線**：`TryReturnTaForm` 加 +5 業力、設 `Flag_HelpedTA_Ch1`（跨章節情分旗標，影響 Ch2/Ch3/Ch4/Ending A）、清除 `Flag_FoundForm`；以 `Flag_HelpedTA_Ch1` 達成冪等。

**購買醜綠傘**：`TryBuyAuntieUglyUmbrella`（`kCh1UglyUmbrellaPrice = 80`）有三個分支：已擁有（冪等）、錢不夠（`ShowMessage`）、購買（扣款 + `SetHeldUmbrella(Ugly)` + 花費提示）。刻意「不」設 `Flag_BoughtUglyUmbrella`（那是 Ch4 集英樓 Vendor 對 Ending-C 的承諾）。

**循序指示燈**：`Ch1IndicatorVisible` 實作三步序列（苦主亮 → 西裝學長亮 → 苦主再亮），以既有旗標為鍵而非名冊 `isQuestGiver` 位，使西裝學長（出貨 `isQuestGiver=false`）在第二步亮燈。

## 關鍵內容（類別 / 函式 / 資料）

- **`void TryReturnVictimUmbrella(EventBus&, Player&, npcId, state)`**：四分支的傘歸還鉤子；授予時不發 `UmbrellaClaimed`（延後至 `LiftChapter1Clear`）。
- **`void TryReturnTaForm(Player&, npcId, state)`**：助教申請書歸還；+5 業力 + `Flag_HelpedTA_Ch1`。
- **`void LiftChapter1Clear(EventBus&, Player&, state, DialogState&)`**：延後章節結束輪詢；對話關閉後發 `ShowMessage` + `UmbrellaClaimed`；`kFlagClearChapter1` 鎖一次性。
- **`bool Ch1IndicatorVisible(npcId, isQuestGiver, Player&)`**：三步序列 NPC 指示燈；以旗標為鍵（不以 `isQuestGiver` 為鍵）。
- **`inline constexpr int kCh1UglyUmbrellaPrice = 80`**：醜綠傘售價，單一來源。
- **`bool TryBuyAuntieUglyUmbrella(EventBus&, Player&, npcId, choiceLabel, state)`**：購買醜綠傘；三分支（已擁有/錢不夠/購買）；不設 `Flag_BoughtUglyUmbrella`。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/quest/Flags.h`（旗標常數）、`include/game/state/SemesterState.h`（章節狀態型別）；前向宣告 `Player`、`EventBus`；`class nccu::DialogState`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（每幀呼叫鉤子）、`src/game/controller/screens/DialogScreen.cpp`（對話確認時呼叫購買鉤子）、`src/game/quest/Chapter1Quest.cpp`（實作）、各任務相關標頭與測試。
- **繼承 / 實作 / 體現**：—（純自由函式宣告，無類別）。
- **每幀管線 / MVC 角色**：Controller 層。`GameController` 在 E 互動管線中呼叫各鉤子；`LiftChapter1Clear` 在每個非對話幀輪詢（緊鄰 `CheckChapterGates` 之前）。

## OO 概念與設計重點

本標頭體現「鉤子（Hook）型 Controller 設計」：每個互動情境對應一個具名自由函式，集中包含所有前置條件檢查與效果，而非在 `GameController` 中使用巨大的 if/switch。每個鉤子內部的多分支（已完成/條件不符/效果落地）形成局部的狀態機，以旗標驅動，具冪等性。

`TryReturnVictimUmbrella` 與 `LiftChapter1Clear` 的「授予 ↔ 延後發事件」分離是核心設計點：先讓 (d) 對話播放，再在對話關閉後觸發轉場。這是 [Observer](../concepts/pat-observer.md) 的延遲語意——事件不在效果發生時立即發布，而是等待正確的敘事時機。`Flag_HelpedTA_Ch1` 的跨章節情分旗標設計體現「玩家選擇的長尾效應」。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/Chapter1Quest.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter1Quest.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
