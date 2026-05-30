# src/ — 實作檔（領域分層）

《尋傘記：政大山下篇》的實作樹。`src/` 與 `include/` 平行鏡射，頂層皆切成
**app / engine / game / ui** 四個領域，相依方向為單向：

```
app  ──▶  game / ui  ──▶  engine
```

- `app` 是組裝層（composition root）與場景切換，知道 game/ui/engine。
- `game` 與 `ui` 屬遊戲層，建立在 `engine` 之上，彼此透過 EventBus / View 介面解耦。
- `engine` 是與本遊戲無關的可重用引擎層，**不**反向相依 game/ui。

本樹共 **80** 個 `.cpp`。檔案數少於 `include/` 是刻意的：`engine` 的 math / render 包裝、
以及 `game/gfx` 多為 header-only，故只有需要存放狀態或非平凡邏輯者才有對應 `.cpp`。

## 目錄樹

```text
.
├── app/                                組裝層與場景生命週期（7）
│   ├── main.cpp                            composition root（刻意置於 app 根）
│   ├── SceneBootstrap.cpp                  場景接線組裝
│   ├── SceneManager.cpp                    場景堆疊管理
│   └── scenes/                             Loading / Title / CharacterSelect / Gameplay 四場景
│
├── engine/                             有狀態 / 需連結 Raylib 的引擎實作（8）
│   ├── audio/    (2)                       AudioDevice / AudioManager
│   ├── events/   (2)                       EventBus / EventSink
│   ├── platform/ (3)                       Harness / ScriptInput / ScriptResolver
│   └── render/   (1)                       RaylibRenderer（其餘 render 包裝為 header-only）
│
├── game/                               遊戲邏輯實作（50）
│   ├── controller/ (12)                    GameController / GameObjectFactory / InputHandler / SceneRouter …
│   │   └── screens/ (4)                        Dialog / Ending / Inventory / Pause 畫面控制
│   ├── dialog/     (6)                     DialogLoader / Source / Opener / Layout / State / View
│   ├── entities/   (13)                    Player / NPC / 雨傘家族 / 消耗品 / 拾取物 / DlcSign
│   ├── quest/      (9)                     各章任務 / ChapterGate / ItemCatalog / QuestHookTable …
│   ├── state/      (2)                     SemesterStateMachine / EndingGate（四結局）
│   ├── vendor/     (2)                     Vendor / VendorLoader
│   └── world/      (6)                     World / TerrainMask / BuildingTracker / WorldSpawn …
│
└── ui/                                 視圖實作（15）
    ├── (扁平視圖 6)                        View / EndingView / InventoryView / MessageView / ChapterCard / HelpPageView
    ├── hud/      (4)                       ObjectiveBar / RainVignette / SportsLapRing / StatusPanel
    ├── overlay/  (3)                       HelpOverlay / MenuAffordance / PauseMenu
    └── world/    (2)                       QuestGiverIndicators / SportsLapTrack
```

> `state/EndingGate` 對應狀態機 **四結局** Ending_A / B / C / D。
