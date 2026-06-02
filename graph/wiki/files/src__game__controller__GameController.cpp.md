---
id: "file:src/game/controller/GameController.cpp"
type: source
path: src/game/controller/GameController.cpp
domain: game
bucket: controller
loc: 244
classes: []
sources: ["src/game/controller/GameController.cpp"]
---
# `GameController.cpp`

> **一句定位**：MVC Controller 的核心：建構模擬管線、接線 EventBus 訂閱者，每幀協調各畫面輸入處理器與 ISystem 管線，以及章節 / 結局閘門的輪詢。

## 職責

`GameController` 是整個遊戲的控制器（MVC 的 C），其建構子完成三件大事：

1. **EventBus 接線**：呼叫 `WireDefaultSubscribers`（章節轉場訂閱）、`WireHudMessageSubscriber`（ShowMessage → World HUD 橫幅）、`WireKarmaToastSubscriber`（KarmaChanged → ShowMessage 轉接），建立從事件到 Model 副作用的完整鏈路。
2. **ISystem 管線建構**：依序 `push_back` `SurvivalSystem`、`MovementSystem`、`CollisionSystem`、`SpawnSystem` 進 `advanceSystems_`——此順序「就是」每幀模擬順序（Survival→移動→碰撞→生成），逐位元一致性已由存檔驗證。
3. **SceneRouter 初始化**：以 `world_.Semester().Current()` 初始化 `sceneRouter_`。

`~GameController()` 呼叫 `bus_.Clear()` 清空訂閱者，防止解構後訂閱 lambda 參照已釋放的 World（先清訂閱再解構 World）。

`Update()` 執行每幀主要邏輯：
- `sceneRouter_.SettleSideEffects(world_)` — 轉場副作用（玩家位置、消耗品清除、詛咒衰減）。
- 四個螢幕處理器提前返回鏈：`HandleEndingMenu()`、`HandlePauseMenu()`、`HandleDialog()`、`HandleInventory()`（優先序由高至低）。
- `Backspace` 略過 HUD 橫幅；`world_.TickHud(dt)` 老化 HUD。
- `SimContext ctx` + for 迴圈跑 `advanceSystems_`。
- `DispatchInteract()` — E 鍵互動分派。
- 建築偵測（`world_.Tracker().Update`）。
- 插曲段出口偵測（`InInterludeExitZone`）。
- `LiftChapter1Clear`、`LiftChapter2Clear`、`CheckChapterGates`、`TryOpenEndingConfession`、`CheckEndingGates`——章節與結局閘門輪詢。
- `sweep_.Run(ctx, dt)` — 幀末延遲刪除。
- `sceneRouter_.SettleRoster(world_)` — 僅供顯示的名冊更新。

四個成員方法（`HandleEndingMenu/PauseMenu/Dialog/Inventory`、`DispatchInteract`）現為薄轉發層，主體移至 `screens/EndingScreen.cpp` 等。

## 關鍵內容（類別 / 函式 / 資料）

- `GameController(World& world, EventBus& bus)` — 建構；WireSubscribers + 建構 ISystem 管線 + SceneRouter。
- `~GameController()` — `bus_.Clear()` 防訂閱者懸空。
- `Update()` — 完整每幀協調者：副作用→提前返回鏈→HUD→SimSystems→Interact→建築→插曲→閘門→Sweep→名冊。
- `advanceSystems_` — `vector<unique_ptr<ISystem>>`；Survival/Movement/Collision/Spawn 四個 ISystem。
- `sweep_` — `SweepSystem` 成員；幀末單獨執行（不在 advanceSystems_ 內，確保在互動/閘門後執行）。
- `sceneRouter_` — `SceneRouter`；管理 FSM 轉場的名冊與副作用結算。
- `pendingVendor_` — `Vendor*`；非擁有指標，由 `DispatchInteract` 設定、`HandleDialog` 讀取並清除。
- `frameColliders_` — `vector<Rect>`，預留 64 容量，CollisionSystem 重用此緩衝區。

## 相依與在架構中的位置
- **#include（往外）**：`World.h`、`Player.h`、`DialogState.h`（Model）；`EventBus.h`（接線）；`EventWiring.h`（WireDefaultSubscribers 等）；`SimSystem.h`（ISystem 管線）；`InteractDispatch.h`、`screens/*`（薄轉發目標）；`ChapterGate.h`、`EndingGate.h`（閘門）；Chapter1-4Quest.h；`SceneRouter.h`、`Vendor.h`、`Physics.h` 等
- **被誰使用（往內）**：—（由 `GameplayScene` 持有為成員）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：MVC Controller；`Update()` 執行完整的 Survival→Movement→Collision→Spawn→Interact→閘門→Sweep 管線

## OO 概念與設計重點

`GameController` 體現 [MVC](../concepts/arch-mvc.md) 的 Controller 角色：唯讀 `World&`（Model）、委託 View 的繪製（不在此處）、協調所有 input + simulation + event 邏輯。[ISystem](../concepts/arch-isystem.md) 管線（`advanceSystems_`）是 [Strategy](../concepts/pat-strategy.md) 模式的應用：每個 ISystem 可獨立替換，且管線次序在建構子一次確立。`bus_.Clear()` 在解構時確保 [Observer](../concepts/pat-observer.md) 訂閱者的生命期不超過 Model，防止 use-after-free。四個螢幕的薄轉發設計（`HandleEndingMenu` 等）是從單一大類別逐步 SRP 抽出的重構路徑，使每個螢幕處理器可獨立測試與閱讀。`TryOpenEndingConfession` 在 `CheckEndingGates` 之前執行體現延後-解算結局的設計：確保玩家先讀到內心獨白，再觸發結局切換（防止突兀跳轉）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/GameController.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/GameController.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [ISystem](../concepts/arch-isystem.md) · [Observer](../concepts/pat-observer.md) · [Strategy](../concepts/pat-strategy.md) · [State](../concepts/pat-state.md)
