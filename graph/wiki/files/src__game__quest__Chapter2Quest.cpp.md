---
id: file:src/game/quest/Chapter2Quest.cpp
type: source
path: src/game/quest/Chapter2Quest.cpp
domain: game
bucket: quest
loc: 223
classes: []
sources: ["src/game/quest/Chapter2Quest.cpp"]
---
# `Chapter2Quest.cpp`

> **一句定位**：第二章（期中考）任務邏輯——圖書館管理員→學霸喚醒→撿筆記→換傘的任務鏈，借傘/歸還，漣漪業力，以及 `!` 指示點亮。

## 職責

本檔實作第二章（`Chapter2_Midterms`）的所有任務鉤子，涵蓋主線任務鏈、輔線借傘、漣漪業力落地，與 `!` 指示可見性。

**`TryMeetLibrarian`**：見到管理員即設 `kFlagMetLibrarian`，解鎖後續與學霸的互動（冪等）。

**`TryRescueBookworm`**：多階段任務機。硬閘控（`!kFlagMetLibrarian` → 「先去問管理員」）。若學霸未喚醒（`!kFlagBookworm`），嘗試消耗 `player.ConsumeOne("EnergyDrink")`：成功則設 `kFlagBookworm`（告知玩家去撿散落筆記）；失敗則提示去地下室自販機補貨（防卡關）。若已喚醒但筆記未完整（`!Chapter2NotesComplete`），提示繼續撿。三份筆記完整後：給 karma +5，`SetHeldUmbrella(True)`，`SetFlag(kFlagBookwormRecovered)`，並清除 `kFlagFoundNote1/2/3`（背包交還筆記）。刻意「不」設 `kFlagHasTrueUmbrella`（Ch4 才讀此旗標）。

**`LiftChapter2Clear`**：每非對話幀輪詢，`kFlagBookwormRecovered` + 對話關閉 + 尚未清關時設 `kFlagCh2Cleared`，觸發 `CheckChapterGates` 的第二章通關轉場。

**`TryLendLibrarianUmbrella`**：學霸喚醒後管理員（`kFlagBookworm` 時的 (b) 路由）。設 `kFlagLibrarianUmbrella` 與 `SetHasUmbrella(true)`（不佔 `heldUmbrella_` 槽，使換回真傘後可與借傘並存，修正「借傘蓋過真傘」缺陷）。

**`TryReturnLibrarianUmbrella`**：僅在 `Interlude_Market` 且 `returnTo==Chapter3`（第二章→第三章插曲段的歸還點）生效。持有借傘時：karma +10，`SetHasUmbrella(keepShelter)`（若同時持真傘則保留遮蔽），清除 `kFlagLibrarianUmbrella`，設 `kFlagLibrarianUmbrellaReturned`（一次性鎖）。

**`Ch2IndicatorVisible`**：管理員未見 → 亮；學霸見管理員後貫穿三個階段亮；Ch2 自販機在「見管理員 + 學霸未喚醒 + 無飲料」時亮（防迷路指引）；其餘維持 `isQuestGiver`。

**`TryApplyCh2Ripple`**：學長 `kFlagHelpedSenior` → karma +3 + 一次性旗標；`kFlagScoldedSenior` → 業力中性（只設一次性旗標）。助教 `kFlagHasProfessorTrap` → karma -10 + 一次性旗標。

## 關鍵內容（類別 / 函式 / 資料）

- `Chapter2NotesComplete(Player&)` → `bool`：三份筆記旗標全具備。
- `TryMeetLibrarian`：設 `kFlagMetLibrarian`（冪等）。
- `TryRescueBookworm`：多階段鏈，消耗能量飲料喚醒，三份筆記換傘，清除筆記旗標。
- `LiftChapter2Clear`：對話後輪詢設 `kFlagCh2Cleared`。
- `TryLendLibrarianUmbrella`：借傘不佔 `heldUmbrella_` 槽。
- `TryReturnLibrarianUmbrella`：市集歸還，karma +10，`keepShelter` 保留真傘遮蔽。
- `Ch2IndicatorVisible`：四路 `!` 點亮邏輯（管理員/學霸/自販機/其他）。
- `TryApplyCh2Ripple`：學長與助教的 Ch2 漣漪業力落地。
- `kNpcCh2Vendor`（引自 `Chapter2Quest.h`）：自販機 NPC id 共用常數。
- `kNpcLibrarianReturn`（引自 `Chapter2Quest.h`）：市集管理員歸還點 NPC id。

## 相依與在架構中的位置

- **#include（往外）**：`Chapter2Quest.h`（旗標常數/`kNpcCh2Vendor`/`kNpcLibrarianReturn`）、`DialogState.h`（`dialog.Active()`）、`EventBus.h`（Publish）、`Player.h`（所有旗標/業力/傘/消耗品操作）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 互動鉤子、`ChapterGate.cpp`、`ChapterVendors.cpp` 引用常數）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：互動鉤子（每幀管線 E 互動階段）與輪詢通關（每非對話幀）兩條路徑均在此。

## OO 概念與設計重點

本檔是最長的任務鏈實作，包含三個設計精要：①「不佔 `heldUmbrella_` 槽的借傘」是對「借傘蓋過真傘」回歸缺陷的結構性修正，利用旗標而非持傘欄位表示借傘；②`TryReturnLibrarianUmbrella` 的 `keepShelter` 判斷確保歸還借傘不會弄丟真傘；③ 漣漪業力各用獨立的一次性旗標，使「學長幫助 +3」與「助教陷阱 -10」恰好各觸發一次且彼此獨立。[Observer](../concepts/pat-observer.md) 貫穿全檔（ShowMessage 事件驅動 HUD）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/Chapter2Quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter2Quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
