## 3. MVC 核心 + ISystem 模擬管線

`World` 是純資料模型（擁有每個 `GameObject`、學期 FSM、建築追蹤、地形碰撞遮罩、對話狀態、
HUD／選單／背包等 UI 狀態），無 raylib、無輸入。`GameController` 收輸入、跑模擬、接事件——
**這次最大的結構改變**：原本約 793 行的 `Update()` god-method 已拆成一條
**`ISystem` 模擬管線**（`SimSystem.h` / `SimSystems.cpp`）。每個 stage 只負責一件事，由
Controller 以固定順序執行，並透過 `SimContext` 串接。這正是 Assignment #6 生存遊戲所需的
可重用 model 端 stage（`CollisionSystem` + `SpawnSystem` 即未來的 Spawner），現在升格為
一等公民型別。E 互動的任務副作用則改用 **資料化的 `QuestHookTable`**（`RunInteractHooks`）
取代約 14 個內嵌 `TryXxx` 呼叫（OCP）。

```mermaid
classDiagram
    direction TB
    class World {
        -objects: ObjectList
        -player: Player*
        -semester: SemesterStateMachine
        -terrainMask: CollisionMask
        -dialog: DialogState
        +Objects() ObjectList&
        +GetPlayer() Player*
        +Sweep()
        +RespawnChapterRoster(state)
        +MaybeSpawnChapter2Notes() bool
        +UpdateSportsLap()
    }
    class GameController {
        -world: World&
        -advanceSystems: vector~ISystem~
        -sweep: SweepSystem
        -sceneRouter: SceneRouter
        -input: InputHandler
        -pendingVendor: Vendor*
        +Update()
        -DispatchInteract()
        -HandleDialog() bool
        -HandleInventory() bool
    }
    class View {
        +Draw(world: const World&)
    }
    class ISystem {
        <<interface>>
        +Run(ctx: SimContext&, dt: float)*
    }
    class SimContext {
        +world: World&
        +worldSize: Vec2
        +playerSize: Vec2
        +frameColliders: vector~Rect~&
        +prevPlayerPos: Vec2
    }
    class SurvivalSystem {
        +Run(ctx, dt)
    }
    class MovementSystem {
        +Run(ctx, dt)
    }
    class CollisionSystem {
        +Run(ctx, dt)
    }
    class SpawnSystem {
        +Run(ctx, dt)
    }
    class SweepSystem {
        +Run(ctx, dt)
    }
    class QuestHookTable {
        <<free functions>>
        +InteractQuestHooks()$ vector~QuestHook~
        +RunInteractHooks(player, npcId, state, returnTo)$
    }

    ISystem <|.. SurvivalSystem
    ISystem <|.. MovementSystem
    ISystem <|.. CollisionSystem
    ISystem <|.. SpawnSystem
    ISystem <|.. SweepSystem
    GameController o-- ISystem : ordered pipeline (1..*)
    GameController ..> SimContext : threads per frame
    GameController --> World : mutates
    GameController ..> QuestHookTable : RunInteractHooks
    SimContext --> World
    View ..> World : reads const
```

### 3a. 周邊服務：EventBus、Vendor、Controller 子助手

```mermaid
classDiagram
    direction LR
    class EventBus {
        <<Singleton>>
        -handlers: map~EventType, Slot[]~
        +Instance()$ EventBus&
        +Subscribe(type, handler) EventBus&
        +ScopedSubscribe(type, handler) Subscription
        +Publish(event: Event) const
        +Clear() EventBus&
    }
    class Subscription {
        <<RAII token>>
        +Reset()
        +Active() bool
    }
    class EventType {
        <<enumeration>>
        UmbrellaClaimed
        KarmaChanged
        ShowMessage
        EnteredBuilding
        PickupAcquired
    }
    class Vendor {
        -config: VendorConfig
        +TryBuy(player, stockIndex) bool
        +IsVendor() bool
    }
    class NPC {
        +Interact(initiator: Player*)
    }
    class InputHandler {
        +TickDialogAdvance(dt) bool
        +ResetDialogAdvance()
    }
    class SceneRouter {
        +SettleSideEffects(world)
        +SettleRoster(world)
    }

    EventBus *-- Subscription : nested RAII token
    EventBus ..> EventType
    NPC <|-- Vendor
    GameController ..> EventBus : publish / wire
    GameController *-- InputHandler
    GameController *-- SceneRouter
    Vendor ..> EventBus : Publish ShowMessage / PickupAcquired
```

> **已知技術債（誠實標註）**：`View::Draw` 仍是一個龐大的單體 renderer（深度排序所有
> 物件＋建築＋裝飾、HUD、對話框、結局畫面、選單皆在其中）；目前透過抽出 `EndingView` /
> `ChapterCard` / `HelpPageView` / `InventoryView` / `MessageView` 等自由函式緩解，但
> `View.cpp` 本體仍偏大。`EventBus` 的 `shared_mutex` 只保護 handler list，handler 本體
> 仍不可跨執行緒 publish（GL 單執行緒）——見 BUGLEDGER H1。

---

[← 回 UML 總覽](README.md) ｜ [上一節：§2 狀態機與結局](2-state-machine.md) ｜ [下一節：§4 gfx 繪圖層 →](4-gfx.md)
