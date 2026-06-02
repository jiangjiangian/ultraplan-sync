---
id: pat-state
type: pattern
title: State
sources: [docs/UML/2-state-machine.md, docs/UML/7-gof.md]
---

# State · 設計模式 (GoF)

> SemesterStateMachine 持有 IChapterState；五個章節狀態切換，四結局以哨兵記錄。

## 是什麼 / 怎麼運作

學期進程是一台 `SemesterStateMachine`，目前章節是一個 `IChapterState`（`Enter/Exit/Update`）。`Transition(next)` 析構舊狀態、建構新的具體章節狀態（Chapter1AddDrop / InterludeMarket / Chapter2Midterms / Chapter3SportsDay / Chapter4Finals）。`Interlude_Market` 是共用轉運站，被重複進出三次，由 `InterludeReturnTo` 決定下一站。四個結局（A→B→D→C）**不是** `IChapterState` 子類別，而以 `ending_`/`inEnding_` 哨兵記錄，判定集中在自由函式 `CheckEndingGates()`，每個非對話幀輪詢一次。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/state/SemesterState.h` | `IChapterState` | [node](../../index.html#node=file:include/game/state/SemesterState.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterState.h) |
| `include/game/state/SemesterStateMachine.h` | `SemesterStateMachine` | [node](../../index.html#node=file:include/game/state/SemesterStateMachine.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterStateMachine.h) |

## 相關概念

[Command / Table（資料化）](pat-command.md) · [ISystem 模擬管線](arch-isystem.md)

## 來源（設計文件）

[`docs/UML/2-state-machine.md`](../../../docs/UML/2-state-machine.md) · [`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=pat-state)