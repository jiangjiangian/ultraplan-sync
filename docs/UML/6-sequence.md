## 6. 系統互動：循序圖（Sequence Diagrams）

### 6a. E 互動 → 多型 BeClaimed → EventBus（保留並更新自原版）

展示玩家按 `E` 與一把未知透明傘互動時，**多型動態綁定 + Observer 解耦** 如何協作。
注意：互動偵測現在發生在 `GameController::DispatchInteract()`（E-probe reach box），
傘的具體後果由 vtable 綁定到 `CursedUmbrella::BeClaimed`，再經 `EventBus` 廣播；物件
不立即刪除，改標記 `isActive_=false`，於幀末 `World::Sweep()` 統一清除。

```mermaid
sequenceDiagram
    participant Input as gfx::Input
    participant GC as GameController
    participant U as TransparentUmbrella*
    participant C as CursedUmbrella (具體實例)
    participant P as Player
    participant Bus as EventBus
    participant Sub as HUD/Toast 訂閱者

    Input->>GC: IsPressed(E)
    GC->>GC: DispatchInteract() 建 E-probe reach box
    Note over GC: ForEachActiveExcept(objects, player, ...)<br/>找到 CheckCollision 命中的 GameObject
    GC->>U: o.AsInteractable()->Interact(player)
    Note over U,C: vtable 動態綁定到 CursedUmbrella
    U->>C: BeClaimed(player)
    C->>P: AddKarma(-30) / SetHeldUmbrella(Cursed)
    C->>Bus: Publish(UmbrellaClaimed)
    C->>Bus: Publish(KarmaChanged)
    C->>Bus: Publish(ShowMessage "順手牽羊…")
    Bus->>Sub: 對 snapshot 後的 handler 廣播 (避免迭代失效)
    Sub-->>Bus: 顯示完成
    C->>C: isActive_ = false (僅標記)
    Note over GC: 幀末 SweepSystem → World::Sweep()<br/>mark-then-sweep，objects_.front() 仍為 Player
```

### 6b. 每幀模擬管線：GameController 依序跑 ISystem（新增）

展示一個「未凍結」幀的 model 推進順序。`SimContext` 由 `MovementSystem` 把 pre-tick 玩家
座標交給 `CollisionSystem`。`SweepSystem` 是終端 stage，在互動／結局判定之後才跑，所以
被某個 gate 標記為 dead 的物件當幀就被回收。

```mermaid
sequenceDiagram
    participant GC as GameController::Update
    participant Sur as SurvivalSystem
    participant Mov as MovementSystem
    participant Col as CollisionSystem
    participant Spw as SpawnSystem
    participant W as World
    participant Gate as EndingGate / ChapterGate
    participant Sw as SweepSystem

    Note over GC: 先跑 4 個輸入處理器<br/>(ending/pause/dialog/inventory)<br/>任何一個 own 此幀即 return (凍結)
    GC->>GC: 建 SimContext{world, sizes, frameColliders}
    GC->>Sur: Run(ctx, dt) — 雨量增/減 (室內 Drain / 撐傘 Sheltered / 露天 Apply)
    GC->>Mov: Run(ctx, dt) — 存 prevPlayerPos + ForEachRole<IUpdatable>
    Mov->>W: 每個 IUpdatable 物件 Update(dt)
    GC->>Col: Run(ctx, dt) — Clamp 世界框 + 逐軸解碰 (BlocksMovement + 地形遮罩)
    GC->>Spw: Run(ctx, dt) — 操場圈 + 4 個自我把關的延後生成
    GC->>GC: DispatchInteract() (E 互動 → RunInteractHooks)
    GC->>Gate: CheckChapterGates / TryOpenEndingConfession / CheckEndingGates
    Gate->>W: 可能 Transition() 學期 FSM
    GC->>Sw: Run(ctx, dt) → World::Sweep() (幀末延後刪除)
    GC->>GC: sceneRouter_.SettleRoster(world) (轉場 roster 同幀生效)
```

---

[← 回 UML 總覽](README.md) ｜ [上一節：§5 autoplay 縫合層](5-harness.md) ｜ [下一節：§7 設計模式對照（GoF） →](7-gof.md)
