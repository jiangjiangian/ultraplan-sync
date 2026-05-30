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

## 各領域子目錄

- **app/**（7）— `main.cpp`（composition root，刻意置於 app 根）+ `SceneBootstrap` /
  `SceneManager`，以及 `scenes/`（Loading / Title / CharacterSelect / Gameplay 四個場景）。
- **engine/**（8）— 有狀態或需連結 Raylib 的引擎實作：
  - `audio/`（2）AudioDevice / AudioManager
  - `events/`（2）EventBus / EventSink
  - `platform/`（3）Harness / ScriptInput / Time
  - `render/`（1）RaylibRenderer（其餘 render 包裝為 header-only）
- **game/**（50）— 遊戲邏輯實作：
  - `controller/`（12）GameController / GameObjectFactory / InputHandler / SceneRouter / EventWiring / SimSystem / screens
  - `dialog/`（6）DialogLoader / Source / Opener / Repository …
  - `entities/`（13）Player / NPC / Item / 雨傘 / 消耗品 / 拾取物 …
  - `quest/`（9）章節任務 / spawn / flags / objective
  - `state/`（2）SemesterStateMachine / EndingGate（**四結局** Ending_A/B/C/D）
  - `vendor/`（2）攤販邏輯
  - `world/`（6）World / Physics / CollisionMask / BuildingTracker
- **ui/**（15）— `View` 與 `hud/`（4）、`overlay/`（3）、`world/`（2）視圖實作，
  外加扁平視圖（EndingView / InventoryView / MessageView / ChapterCard / HelpPageView）。
