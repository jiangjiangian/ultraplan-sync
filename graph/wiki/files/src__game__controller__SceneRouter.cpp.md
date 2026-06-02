---
id: "file:src/game/controller/SceneRouter.cpp"
type: source
path: src/game/controller/SceneRouter.cpp
domain: game
bucket: controller
loc: 102
classes: []
sources: ["src/game/controller/SceneRouter.cpp"]
---
# `SceneRouter.cpp`

> **一句定位**：FSM 轉場的雙段結算：`SettleSideEffects`（Update 開頭可觀察副作用）與 `SettleRoster`（Update 結尾名冊更新），以兩個游標守門確保每章進入僅觸發一次，且 npcs[] 同幀更新。

## 職責

`SceneRouter` 將章節轉場的副作用拆成兩個時機：

**`SettleSideEffects(World&)`**（Update 開頭執行）：以 `lastRosterState_` 游標守門，每章進入僅觸發一次。動作：
- 若名冊尚未重生，補做 `RespawnChapterRoster`（冪等雙重保險）。
- **Interlude_Market 抵達**：設玩家位置至 `kInterludeEntry`；清空消耗品（`ClearConsumables`，「消耗品當章用完」機制）；發布 `ShowMessage(kInterludeArrivalHint)`；重置 `interludeExitZoneLatched_`。
- **Chapter2 / 3 / 4 進入**：`SetHasUmbrella(false)` + `ClearFlag(kFlagHasTrueUmbrella)` + `ClearFlag(kFlagLibrarianUmbrella)`（每章從空手開始，背包與「傘又掉了」字卡一致）；`ApplyCursedTaintDecay()`（詛咒污染每章 -5 × taint，非零才 AddKarma 避免影響存檔一致性）。

**`SettleRoster(World&)`**（Update 結尾執行）：以 `lastRosterRespawnState_` 游標守門。動作：偵測到 FSM 狀態改變時呼叫 `world.RespawnChapterRoster(cur)`，使 View 在轉場幀就畫出新章節的 NPC 名冊，消除 npcs[] 落後一幀的問題。純資料變動，不發事件。

`interludeExitZoneLatched_` 是插曲段離開提示的閂鎖（由 `GameController` 的 `MaybeAnnounceInterludeExit` 讀寫），`InterludeExitLatchMut()` 提供可變參照。

## 關鍵內容（類別 / 函式 / 資料）

- `SettleSideEffects(World&)` — 游標守門；Interlude 抵達副作用；Ch2/3/4 進入的傘清除 + 詛咒衰減。
- `SettleRoster(World&)` — 游標守門；`RespawnChapterRoster`；純顯示更新。
- `lastRosterState_` / `lastRosterRespawnState_` — 兩個 `SemesterState` 游標，防止重複執行。
- `interludeExitZoneLatched_` — 插曲段離開帶狀閂鎖；`InterludeExitLatchMut()` 提供可變參照。
- `kInterludeEntry` — 插曲段玩家入口位置常數（來自 `InterludeExit.h`）。

## 相依與在架構中的位置
- **#include（往外）**：`SceneRouter.h`；`Flags.h`（旗標名稱）；`ChapterToast.h`（toast 訂閱）；`EventBus.h`；`InterludeExit.h`（`kInterludeEntry`、`kInterludeArrivalHint`）；`Player.h`；`SemesterState.h`；`World.h`
- **被誰使用（往內）**：—（由 `GameController` 持有為成員，每幀呼叫 SettleSideEffects / SettleRoster）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；SettleSideEffects 在 Update 最前端，SettleRoster 在 Sweep 之後；兩端合圍轉場的觀察窗口

## OO 概念與設計重點

「雙段結算」設計是刻意的時序拆分，解決了 harness 存檔可觀察時間線（SettleSideEffects 保持逐位元一致）與 View 名冊顯示延遲（SettleRoster 消除落後一幀）之間的矛盾——兩者互不干擾。游標（`lastRosterState_` / `lastRosterRespawnState_`）確保冪等性：即使某段程式碼重複呼叫也不會雙重執行。`ApplyCursedTaintDecay()` 僅在污染 > 0 時呼叫 `AddKarma`（「非零才觸發 KarmaChanged 事件」），體現對 harness 存檔逐位元一致性的精細管理。此類別與 [State](../concepts/pat-state.md) 模式的狀態機（`SemesterStateMachine`）直接交互，觀察 FSM 的 `Current()` 並執行對應的進場副作用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/SceneRouter.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/SceneRouter.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md) · [MVC](../concepts/arch-mvc.md)
