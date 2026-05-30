# include/ — 公開標頭檔（領域分層）

《尋傘記：政大山下篇》的標頭檔樹。`include/` 與 `src/` 平行鏡射，頂層皆切成
**app / engine / game / ui** 四個領域，相依方向為單向：

```
app  ──▶  game / ui  ──▶  engine
```

- `app` 是組裝層（composition root）與場景切換，知道 game/ui/engine。
- `game` 與 `ui` 屬遊戲層，建立在 `engine` 之上，彼此盡量解耦（透過 EventBus / View 介面）。
- `engine` 是與本遊戲無關的可重用引擎層，**不**反向相依 game/ui。

本樹共 **144** 個標頭檔。多數 `engine`（math / render 包裝）與 `game/gfx` 屬 header-only，
因此 `include/` 的檔案數高於 `src/`。

## 目錄樹

```text
.
├── app/                                程式進入點與場景生命週期（7）
│   ├── IScene.h                            場景介面（update / draw / 切換）
│   ├── SceneManager.h                      場景堆疊管理
│   ├── SceneBootstrap.h                    組裝各場景的進入點接線
│   └── scenes/                             Loading / Title / CharacterSelect / Gameplay 四場景
│
├── engine/                             與遊戲無關的可重用引擎層（27）
│   ├── audio/    (2)                       AudioDevice / AudioManager
│   ├── core/     (2)                       GameObject 基底 + Roles（角色能力介面）
│   ├── events/   (3)                       EventBus / EventSink / HudSlot（發布訂閱骨幹）
│   ├── input/    (2)                       Input / Key（輸入抽象）
│   ├── math/     (3)                       Vec2 / Rect / Color（純值型別）
│   ├── platform/ (4)                       Harness / ScriptInput / Time / WorkingDir
│   └── render/   (11)                      Raylib 薄包裝：Window / Texture / Font / Camera2D / IRenderer …
│
├── game/                               遊戲邏輯（89）
│   ├── controller/ (14)                    GameController / GameObjectFactory / InputHandler / SceneRouter …
│   │   └── screens/ (4)                        Dialog / Ending / Inventory / Pause 畫面控制
│   ├── dialog/     (7)                     DialogLoader / Layout / View / State / Source / Opener / Repository
│   ├── entities/   (17)                    Player / NPC / Character / Item / 雨傘家族 / 消耗品 / 拾取物 / DlcSign
│   ├── gfx/        (6)                     繪製輔助（header-only）：Bounds / SpriteStrip / WalkCycle …
│   ├── quest/      (17)                    章節任務 / spawn / flags / objective / QuestHookTable
│   ├── state/      (13)                    SemesterStateMachine + 各章 State + Interlude + EndingGate（四結局）
│   ├── vendor/     (5)                     攤販 Vendor / Config / Loader / Messages / Sprite
│   └── world/      (10)                    World / Physics / CollisionMask / BuildingTracker / 地形
│
└── ui/                                 呈現層（21）
    ├── (扁平視圖 12)                       View / CharacterSelect / EndingView / InventoryView / MessageView …
    ├── hud/      (4)                       ObjectiveBar / RainVignette / SportsLapRing / StatusPanel
    ├── overlay/  (3)                       HelpOverlay / MenuAffordance / PauseMenu
    └── world/    (2)                       QuestGiverIndicators / SportsLapTrack
```

> `state/` 的 `EndingGate` 對應狀態機 **四結局** Ending_A / B / C / D。
