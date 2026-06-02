---
id: file:include/game/state/InterludeMarket.h
type: header
path: include/game/state/InterludeMarket.h
domain: game
bucket: state
loc: 28
classes: [InterludeMarket]
sources: ["include/game/state/InterludeMarket.h"]
---
# `InterludeMarket.h`

> **一句定位**：State 模式的幕間市集具體狀態物件，回報 `Id()` 為 `Interlude_Market` 與顯示名稱「幕間 市集」，是章節之間玩家採買的過場狀態。

## 職責

`InterludeMarket.h` 定義繼承自 `IChapterState` 的 `InterludeMarket` 類別，是章節清關後進入的過場狀態物件（State 模式的葉節點）。

章節清關後進入幕間市集；玩家在此向攤販採買，往南走進 `InInterludeExitZone` 後由 `ChapterGate`/`SceneRouter` 消費 `Flag_LeaveInterlude`，接著以 `SemesterStateMachine::InterludeReturnTo()` 的目標返回下一章。`InterludeMarket` 只回報 `Id()` 與 `Name()`；實際的市集邏輯（攤販生成、出口判定、返回目標）分散在 `ChapterVendors`、`InterludeExit`、`SemesterStateMachine::SetInterludeReturnTo` 中。

## 關鍵內容（類別 / 函式 / 資料）

- `class InterludeMarket : public IChapterState`：28 行薄包裝。
- `Id() const → SemesterState`：回傳 `SemesterState::Interlude_Market`。
- `Name() const → string_view`：回傳 `"幕間 市集"`。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`
- **被誰使用（往內）**：`src/game/state/SemesterStateMachine.cpp`
- **繼承 / 實作 / 體現**：繼承自 `IChapterState`
- **每幀管線 / MVC 角色**：Model 層 State 機的身分持有物；實際的市集邏輯由 `ChapterVendors`（攤販配置）、`InterludeExit`（出口觸發）與 `SemesterStateMachine::InterludeReturnTo`（下一章目標）協作驅動。

## OO 概念與設計重點

[State 模式](../concepts/pat-state.md)標準葉節點。幕間市集作為「共用轉運站」（SPEC.md 中提到的 `Interlude_Market` 共用轉運站角色）在 `SemesterStateMachine` 的設計中佔有特殊位置：`interludeReturnTo_` 儲存在狀態機而非狀態物件本身，恰好是因為 `InterludeMarket` 物件在每次 `Transition()` 時都會被銷毀重建，跨轉移的返回目標無法儲存在短暫的狀態物件中。這個設計決策在 `SemesterStateMachine.h` 的注釋中有明確說明。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/InterludeMarket.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeMarket.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
