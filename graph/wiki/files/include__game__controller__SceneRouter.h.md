---
id: "file:include/game/controller/SceneRouter.h"
type: header
path: include/game/controller/SceneRouter.h
domain: game
bucket: controller
loc: 110
classes: [SceneRouter]
sources: ["include/game/controller/SceneRouter.h"]
---
# `SceneRouter.h`

> **一句定位**：觀察 `SemesterState` FSM 轉場、讓 World 跟隨 FSM 的觀察者——以「分拆兩半部」的手法解決一幀延遲的名冊/座標不一致問題。

## 職責

`SceneRouter` 從 `GameController` 中抽出的類別，職責是「讓 World 跟隨 FSM 轉場」：偵測 `SemesterState` 相對上次觀察值是否改變，並依目的地套用對應的副作用（NPC 名冊重生、玩家重新定位、消耗品清空、Ch4 雨傘重置、離開閂鎖重置、抵達市集的 ShowMessage）。

最核心的設計決策是**兩個入口點**，解決了一個早期版本中存在的一幀延遲視覺問題（轉場觸發後下一幀才套用名冊，導致畫面出現 `semester=新` 但 `npcs[]=舊` 的不一致幀）：

- **`SettleRoster(World&)`**：只做 NPC 名冊抽換。於 `Update()` 結尾呼叫，確保轉場發生的那一幀就畫出新名冊。不寫玩家座標、不清消耗品——那些可觀察副作用若在轉場幀更動會破壞腳本（已驗證：直接傳送玩家的版本會卡住某結局流程）。追蹤獨立游標 `lastRosterRespawnState_`。

- **`SettleSideEffects(World&)`**：另一半（玩家座標/消耗品/旗標/提示/閂鎖）。於 `Update()` 開頭呼叫，與早期內嵌區塊位置相同，維持可觀察時間軸不變。推進 `lastRosterState_`；若 `SettleRoster` 因故被略過，防禦性地一併重生名冊。

兩個游標（`lastRosterState_`/`lastRosterRespawnState_`）讓兩半部各自冪等，可單獨呼叫。市集離開區提示閂鎖（`interludeExitZoneLatched_`）透過 `InterludeExitLatchMut()` 暴露給 `GameController`，集中管理所有轉場相關狀態。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class SceneRouter` | 禁止拷貝；建構子以初始 `SemesterState` 初始化兩個游標。 |
| `explicit SceneRouter(SemesterState initial)` | 以 `initial` 同時設定兩個游標（`lastRosterState_` 和 `lastRosterRespawnState_`）。 |
| `SettleRoster(World&)` | 幀末名冊抽換；追蹤 `lastRosterRespawnState_`。 |
| `SettleSideEffects(World&)` | 幀頭副作用（座標/消耗品/旗標/提示/閂鎖）；追蹤 `lastRosterState_`。 |
| `InterludeExitLatchMut()` → `bool&` | 市集離開區提示閂鎖的可變參考。 |
| `LastRosterState()` | 測試用：上次副作用對應的 FSM 狀態。 |
| `LastRosterRespawnState()` | 測試用：上次名冊重生對應的 FSM 狀態。 |
| `lastRosterState_` | `SettleSideEffects` 的游標。 |
| `lastRosterRespawnState_` | `SettleRoster` 的游標（獨立於前者）。 |
| `interludeExitZoneLatched_` | 市集離開提示的每次到訪一次閂鎖；市集抵達時重置。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/state/SemesterState.h`（`SemesterState` 列舉；游標型別）。
- **被誰使用（往內）**：`include/game/controller/GameController.h`（作為成員 `sceneRouter_`）、`src/game/controller/SceneRouter.cpp`（兩個方法的實作）、`src/game/controller/screens/DialogScreen.cpp`（對話確認後呼叫 `SettleRoster`）、`tests/controller/test_scene_router.cpp`、`tests/quest/test_chapter_transitions.cpp`（單元/整合測試）。
- **繼承 / 實作 / 體現**：—（觀察者模式的輕量實現，無繼承）
- **每幀管線 / MVC 角色**：Controller 層的 FSM 轉場觀察者。`SettleSideEffects` 在每幀開頭（管線前）執行，`SettleRoster` 在每幀結尾（清除前）執行，分別對應管線的兩端。

## OO 概念與設計重點

`SceneRouter` 體現了輕量版的 [Observer（pat-observer）](../concepts/pat-observer.md) 模式：它「輪詢」（polling observer）FSM 狀態而非訂閱事件，以兩個游標追蹤已觀察值，偵測差異後套用副作用。相對於事件驅動版本，輪詢觀察者更容易控制執行時機（幀頭/幀尾），是解決「一幀延遲」問題的關鍵設計選擇。

「兩半部分拆」的設計是本類別最精妙的地方：它以「時機控制」（名冊在幀末、副作用在幀頭）取代了更複雜的事務性解法，且維持了可觀察行為的向後相容（與早期內嵌版本的時間軸相同）。這是一個「修復時間問題不是靠更多狀態，而是靠更精確的執行時機」的設計範例。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/SceneRouter.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SceneRouter.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
