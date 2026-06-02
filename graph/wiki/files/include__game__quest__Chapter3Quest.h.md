---
id: file:include/game/quest/Chapter3Quest.h
type: header
path: include/game/quest/Chapter3Quest.h
domain: game
bucket: quest
loc: 97
classes: []
sources: ["include/game/quest/Chapter3Quest.h"]
---
# `Chapter3Quest.h`

> **一句定位**：Ch3「校慶運動會」物物交換鏈任務的旗標說明、操場幾何常數宣告、三個互動鉤子與循序指示燈閘門。

## 職責

本標頭是 Ch3（`Chapter3_SportsDay`）任務鏈的公開 API，結合了「任務邏輯」與「操場場地幾何」兩類常數。

**物物交換鏈**：`TryAdvanceCh3Trade` 是三環的循序互動鉤子。以旗標狀態決定目前在哪一環：A 系烤香腸攤主（`Flag_HasSausage`，+3 業力）→ B 系大聲公持有者（清香腸+設Loudspeaker，+3 業力）→ C 系學姊（清大聲公+設 `KnowsUmbrellaLoc`，+5 業力）。每環的守衛（剛設的旗標擋下下次對話）確保恰好觸發一次。道具以「旗標」建模而非計數消耗品（一次性任務物品模式）。交換鏈是業力/敘事路徑，不是硬性閘門——Ch3 真正結束由 `TransparentUmbrella::BeClaimed` 觸發。

**ProfessorTrap 漣漪**：`TryApplyCh3Ripple` 落地「持 ProfessorTrap 進入體育館被認出」的 -10 業力，以 `kFlagCh3RippledProfTrap` 每 Ch3 一次性。此旗標與 `kFlagCh2RippledTA` 分開，使 Ch3 的 -10 即便 Ch2 已扣過仍獨立計算。

**操場幾何常數**：`kChapter3UmbrellaPos`（真傘生成位置 1320,520，已遮罩驗證可走）、`kSportsTrackCx/Cy`（操場中心 1694,740）、`kSportsTrackHalfLen`（直線段半長 150.0f）、`kSportsTrackR`（端帽半徑 130.0f）——跑道為「田徑場」形狀（直線+端帽）。這些常數是跑圈進度追蹤（`WorldSportsLap`）、人群跑者生成（`WorldSpawn`）與跑道環繪製（View）的單一事實來源。

**循序指示燈**：`Ch3IndicatorVisible` 依交換鏈旗標讓三盞燈依序揭露；非鏈 NPC 一律回傳 true（保持獨立）。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline constexpr Vec2 kChapter3UmbrellaPos{1320.0f, 520.0f}`**：Ch3 真傘出現位置（體育館左側）。
- **`inline constexpr float kSportsTrackCx = 1694.0f` / `kSportsTrackCy = 740.0f`**：操場中心。
- **`inline constexpr float kSportsTrackHalfLen = 150.0f` / `kSportsTrackR = 130.0f`**：跑道幾何（直線半長 + 端帽半徑）。
- **`void TryAdvanceCh3Trade(EventBus&, Player&, npcId, state)`**：三環物物交換鏈；以旗標決定當前環；每環一次性。
- **`bool Ch3IndicatorVisible(npcId, Player&)`**：交換鏈循序指示燈；非鏈 NPC 一律 true。
- **`void TryApplyCh3Ripple(EventBus&, Player&, state)`**：落地 ProfessorTrap 漣漪 -10 業力；`kFlagCh3RippledProfTrap` 每 Ch3 一次性。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/quest/Flags.h`、`include/game/state/SemesterState.h`、`include/engine/math/Vec2.h`（座標型別）；前向宣告 `Player`、`EventBus`。
- **被誰使用（往內）**：`include/game/gfx/Decorations.h`（引用 `kSportsTrackCy`）；`src/game/controller/GameController.cpp`、`src/game/dialog/DialogOpener.cpp`、`src/game/quest/Chapter3Quest.cpp`（實作）、`src/game/world/WorldSpawn.cpp`、`src/game/world/WorldSportsLap.cpp`、`src/ui/world/SportsLapTrack.cpp`；多個測試。
- **繼承 / 實作 / 體現**：—（純自由函式宣告 + 常數，無類別）。
- **每幀管線 / MVC 角色**：Controller 層互動鉤子；`WorldSportsLap` 每幀讀 `kSportsTrack*` 常數計算跑圈進度；View 以 `kSportsTrack*` 繪製跑道環。

## OO 概念與設計重點

Ch3 標頭的特殊之處是「任務邏輯 + 場地幾何」共存：操場幾何常數讓跑道相關的四個子系統（跑者 NPC、進度追蹤、裝飾座標、跑道環繪製）共用單一事實來源，消除漂移。這比把常數分散在各個使用端更容易維護。

`TryAdvanceCh3Trade` 的「道具以旗標建模」選擇（而非計數消耗品）強調這些物品是一次性任務媒介，不應出現在背包計數中。交換鏈不是硬性閘門的決策讓探索式玩法（略過交換鏈直接找傘）成為可能，業力差異提供動機而非強制。`TryApplyCh3Ripple` 的每章獨立鎖鍵設計體現「跨章節業力計算的精確性」——相同 NPC 的漣漪在 Ch2 和 Ch3 是獨立的兩次，而非同一個。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/Chapter3Quest.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter3Quest.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
