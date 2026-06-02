---
id: file:src/game/quest/Chapter3Quest.cpp
type: source
path: src/game/quest/Chapter3Quest.cpp
domain: game
bucket: quest
loc: 128
classes: []
sources: ["src/game/quest/Chapter3Quest.cpp"]
---
# `Chapter3Quest.cpp`

> **一句定位**：第三章（校慶）任務邏輯——A→B→C 物物交換鏈的推進、依序 `!` 點亮，以及 Ch1 漣漪在 Ch3 的業力落地。

## 職責

本檔實作第三章（`Chapter3_SportsDay`）的三個函式：物物交換鏈推進（`TryAdvanceCh3Trade`）、`!` 指示可見性（`Ch3IndicatorVisible`）、漣漪業力落地（`TryApplyCh3Ripple`）。

**`TryAdvanceCh3Trade`**：A→B→C 三環鏈的核心推進機。

- **A 系（`vendor_sausage_a`）**：若鏈尚未開始（無 `HasSausage/HasLoudspeaker/KnowsUmbrellaLoc`），先檢查 `kFlagSportsLapDone`（必須先跑一圈）；通過後 karma +3 + `SetFlag(kFlagHasSausage)`，發布「A 系攤主塞給你一根烤香腸」。
- **B 系（`loudspeaker_b`）**：若已推進過第二環（`loud || known`）返回。否則若手上無香腸，指回 A 系（順序強制）。有香腸時：清除 `kFlagHasSausage`，karma +3 + `SetFlag(kFlagHasLoudspeaker)`。
- **C 系（`senior_c`）**：情報揭露環節，karma +5 的最大獎勵。若已知傘位則返回；無大聲公則指回 B 系。有大聲公時：清除 `kFlagHasLoudspeaker`，設 `kFlagKnowsUmbrellaLoc`，發布傘在「體育館後台道具箱第三個」的情報。

**`Ch3IndicatorVisible`**：A 在鏈頭亮（含未跑圈前，給出目標讓 A 的「先跑圈」轉向提示情境教導第一步）；B 在持香腸時亮；C 在持大聲公時亮。以此確保三者輪流亮起而非同時全亮。

**`TryApplyCh3Ripple`**：
- 漣漪一（陷阱傘 -10）：`kFlagHasProfessorTrap` + `!kFlagCh3RippledProfTrap` → karma -10 + 一次性旗標。
- 漣漪二（助教情分 +5）：`kFlagHelpedTACh1` + `!kFlagCh3RippledTAHelped` → karma +5 + 一次性旗標。兩漣漪互相獨立，可在同一輪各自恰好觸發一次。

## 關鍵內容（類別 / 函式 / 資料）

- `TryAdvanceCh3Trade(EventBus&, Player&, npcId, state)`：三環鏈，香腸→大聲公→情報。
- `Ch3IndicatorVisible(npcId, Player&)` → `bool`：三 NPC 的依序 `!` 點亮邏輯。
- `TryApplyCh3Ripple(EventBus&, Player&, state)`：陷阱傘 -10 + 助教情分 +5，各用獨立一次性旗標。
- `kFlagSportsLapDone` / `kFlagHasSausage` / `kFlagHasLoudspeaker` / `kFlagKnowsUmbrellaLoc`：三環鏈旗標。
- `kFlagCh3RippledProfTrap` / `kFlagCh3RippledTAHelped`：獨立漣漪一次性鍵。

## 相依與在架構中的位置

- **#include（往外）**：`Chapter3Quest.h`（所有旗標常數）、`EventBus.h`（Publish）、`Player.h`（旗標/業力操作）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 互動鉤子與輪詢呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：互動鉤子（每幀管線 E 互動階段）。`TryApplyCh3Ripple` 由 `GameController` 在 Ch3 章節進場時呼叫（非互動路徑）。

## OO 概念與設計重點

物物交換鏈是「順序強制的任務鏈」設計：每環都有「錯序則指回前環」的防錯邏輯，使玩家永遠知道下一步要去哪裡（Chain-of-Responsibility 的序列形式）。`Ch3IndicatorVisible` 的「A 在跑圈前就亮」是刻意的 UX 決策（說明在行內），防止玩家進入章節時看到空曠地圖而迷失。兩個漣漪以獨立的一次性旗標確保不重疊觸發，遵循旗標驅動系統的基本不變式。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/Chapter3Quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter3Quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
