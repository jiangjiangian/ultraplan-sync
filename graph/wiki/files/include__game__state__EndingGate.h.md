---
id: file:include/game/state/EndingGate.h
type: header
path: include/game/state/EndingGate.h
domain: game
bucket: state
loc: 32
classes: []
sources: ["include/game/state/EndingGate.h"]
---
# `EndingGate.h`

> **一句定位**：宣告 `CheckEndingGates` 自由函式，是第四章每幀輪詢「是否達成某結局條件」並驅動學期狀態機轉移的唯一入口。

## 職責

`EndingGate.h` 宣告單一函式 `CheckEndingGates(bus, player, semester, dialog)`，作為四結局條件判定的集中入口。此函式在 Ch4 `Chapter4_Finals` 期間每個「非對話幀」由 `GameController` 呼叫一次。

判定優先序 A→B→D→C：先命中者勝。一旦 `Flag_TaFinaleChoiceMade` 設立，閘門即為「完全」，四者必有其一觸發（不會卡死）。結局觸發來源有兩類：碰觸雨傘（詛咒傘→B，醜傘→C），或助教終局選擇（體諒→A，風雨同行→D，質問冷淡→B）。當對話仍 `Active()` 時延後判定（讓收尾的自白/旁白先讀完），轉移當下會關閉對話。

`EventBus` 由外部注入（不取全域 `Instance()`），`DialogState` 傳入以供 `Active()` 判定與轉移時 `Close()`，`SemesterStateMachine` 傳入以供 `Transition()`。這使函式的所有相依均明確，無全域副作用，便於單元測試。

## 關鍵內容（類別 / 函式 / 資料）

- `CheckEndingGates(EventBus& bus, Player& player, SemesterStateMachine& semester, DialogState& dialog)`：四結局條件判定與轉移的全部邏輯入口；實作在 `EndingGate.cpp`。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 #include 依賴）；以前向宣告引用 `Player`、`EventBus`、`nccu::SemesterStateMachine`、`nccu::DialogState`
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（每幀呼叫）、`src/game/controller/screens/DialogScreen.cpp`、`src/game/state/EndingGate.cpp`（實作體）；測試（`test_ch4_ending_confession.cpp`、`test_ch4_finale.cpp`、`test_ch4_gentle_umbrella.cpp`、`test_chapter_spine.cpp`、`test_chapter_transitions.cpp`、`test_ending_gate.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 管線——在 Spawn→Collision→互動鉤子之後、Sweep 前的結局判定步驟；僅在 `Current() == Chapter4_Finals` 且對話未 `Active()` 時實際執行判定。

## OO 概念與設計重點

前向宣告替代 #include 的設計使此標頭極度輕量（32 行，無任何 #include 指令），減少標頭相依圖中的複雜度，同時讓 `EndingGate.cpp` 的實作可以完全獨立演進而不影響 include 鏈。所有相依透過參數傳入（依賴注入），使函式的行為完全由參數決定，便於測試與替換。`Flag_TaFinaleChoiceMade` 作為「完全閘門」的設計確保遊戲不會出現「活鎖」（所有結局條件都未達成但玩家已觸發終局選擇）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/EndingGate.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/EndingGate.h) · [← 全檔索引](../files-index.md)
