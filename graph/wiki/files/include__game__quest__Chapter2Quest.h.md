---
id: file:include/game/quest/Chapter2Quest.h
type: header
path: include/game/quest/Chapter2Quest.h
domain: game
bucket: quest
loc: 211
classes: []
sources: ["include/game/quest/Chapter2Quest.h"]
---
# `Chapter2Quest.h`

> **一句定位**：Ch2「圖書館期中考」任務的旗標說明文件與七個互動鉤子宣告——學霸救援雙階段、管理員相遇/借傘/歸還、Ch1→Ch2 漣漪業力、指示燈閘門、與延後章節結束輪詢。

## 職責

本標頭是 Ch2（`Chapter2_Midterms`）任務鏈的完整公開 API，記錄所有旗標的語意並宣告七個互動鉤子。

**核心救援流程**：`TryRescueBookworm` 是雙階段鉤子：第一階段（沉睡）消耗背包中的 EnergyDrink → 設 `Flag_Bookworm`，學霸醒來請玩家撿筆記；第二階段（已喚醒）三頁齊備後觸發筆記 ↔ 傘的交換（+5 業力）。刻意不設 `Flag_Ch2Cleared`，延後到 `LiftChapter2Clear`。

**管理員鏈**：`TryMeetLibrarian`（第一次對話設 `kFlagMetLibrarian`，解鎖學霸路徑）→ `TryLendLibrarianUmbrella`（以 `Flag_Bookworm` 把守，授予借傘且刻意不佔用 `heldUmbrella_` 槽，使真傘可與借傘並存）→ `TryReturnLibrarianUmbrella`（Ch2→Ch3 Interlude 歸還，+10 業力；略過者借傘在進入 Ch3 時自動清除）。

**反死鎖設計**：Ch2 散落零錢（ChapterPickups）共計 40 元，大於圖書館地下室自販機的飲料售價 35 元，確保身無分文的玩家仍能湊出喚醒學霸的必要道具——chapter2.md 承諾的反死鎖底線在資料層有具體實現。

**漣漪業力**：`TryApplyCh2Ripple` 落地 Ch1 旗標驅動的反應式業力（西裝學長 ±3、助教 -10），以每 NPC 一次性鎖鍵確保不重複。`Chapter2NotesComplete` 查詢三頁筆記是否齊備，作為多處前置條件檢查的共用工具。

兩個 `constexpr` 字串常數提供單一真實來源：`kNpcLibrarianReturn`（歸還點 NPC ID）、`kNpcCh2Vendor`（自販機 NPC ID）。

## 關鍵內容（類別 / 函式 / 資料）

- **`bool Chapter2NotesComplete(const Player&)`**：查詢三頁筆記（`Flag_FoundNote1/2/3`）是否全部設立。
- **`void TryRescueBookworm(EventBus&, Player&, npcId, state)`**：雙階段救援鉤子；第一階段消耗 EnergyDrink；第二階段三頁齊備後交換。
- **`void LiftChapter2Clear(Player&, state, DialogState&)`**：延後章節結束；對話關閉後設 `Flag_Ch2Cleared`。
- **`void TryMeetLibrarian(Player&, npcId, state)`**：設 `kFlagMetLibrarian`，解鎖學霸路徑；冪等。
- **`void TryLendLibrarianUmbrella(Player&, npcId, state)`**：借傘（不佔 `heldUmbrella_` 槽）；`kFlagLibrarianUmbrella` 冪等。
- **`void TryReturnLibrarianUmbrella(EventBus&, Player&, npcId, state, returnTo)`**：歸還借傘 +10 業力；三分支（情境不符/已歸還/歸還）；歸還不弄丟真傘。
- **`void TryApplyCh2Ripple(Player&, npcId, state)`**：落地 Ch1 旗標驅動的反應式漣漪業力；每 NPC 一次性。
- **`bool Ch2IndicatorVisible(npcId, isQuestGiver, Player&)`**：四步循序指示燈（管理員→學霸→自販機中繼→學霸完成）；以 npcId 為鍵（不以 isQuestGiver 為鍵）。
- **`inline constexpr const char* kNpcLibrarianReturn = "librarian_return"`**：歸還點 NPC ID。
- **`inline constexpr const char* kNpcCh2Vendor = "ch2_vending"`**：自販機 NPC ID。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/quest/Flags.h`、`include/game/state/SemesterState.h`；前向宣告 `Player`、`EventBus`；`class nccu::DialogState`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`、`src/game/dialog/DialogOpener.cpp`、`src/game/quest/Chapter2Quest.cpp`（實作）、`src/game/quest/ChapterVendors.cpp`（以 `kNpcCh2Vendor` 設攤販）、`src/game/world/World.cpp`/`WorldSpawn.cpp`（生成管理）；大量測試。
- **繼承 / 實作 / 體現**：—（純自由函式宣告，無類別）。
- **每幀管線 / MVC 角色**：Controller 層。`GameController` 在 E 互動管線呼叫各鉤子；`LiftChapter2Clear` 在每個非對話幀輪詢（緊鄰 `CheckChapterGates` 之前）。

## OO 概念與設計重點

Ch2 任務鏈展示了「任務狀態以旗標編碼」的設計哲學：沒有顯式的任務狀態機物件，所有分支邏輯由 `Player::HasFlag(...)` 決策，使任何鉤子函式都能在任意時序下安全呼叫而不導致狀態不一致。

`TryLendLibrarianUmbrella` 「不佔用 `heldUmbrella_` 槽」的決策是細緻的設計：允許借傘（純旗標表示）與真傘（`heldUmbrella_` + 旗標）並存，使稍後「換回真傘」的背包列不覆蓋借傘列。`TryApplyCh2Ripple` 的每 NPC 獨立一次性鎖鍵是「漣漪業力精確計算」的保障——同一個 Ch2 同一個 NPC 只落地一次，但多個 NPC 的漣漪相互獨立。

[Observer](../concepts/pat-observer.md) 的事件延遲語意再次出現（`LiftChapter2Clear` 等對話關閉後才設旗標），與 Ch1 同構。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/Chapter2Quest.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter2Quest.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
