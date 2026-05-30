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

## 各領域子目錄

- **app/**（7）— 程式進入點與場景生命週期：`IScene` 介面、`SceneManager`、`SceneBootstrap`，
  以及 `scenes/`（Loading / Title / CharacterSelect / Gameplay 四個場景）。
- **engine/**（27）— 與遊戲無關的引擎層，再分為：
  - `audio/`（2）AudioDevice / AudioManager
  - `core/`（2）GameObject 基底與 Roles（角色能力介面）
  - `events/`（3）EventBus / EventSink / HudSlot（發布訂閱解耦骨幹）
  - `input/`（2）Input / Key（輸入抽象）
  - `math/`（3）Vec2 / Rect / Color（純值型別）
  - `platform/`（4）Harness / ScriptInput / Time / WorkingDir（平台與自動遊玩感知層）
  - `render/`（11）Raylib 薄包裝：Window / Texture / Font / Camera2D / IRenderer / RaylibRenderer …
- **game/**（89）— 遊戲邏輯，再分為：
  - `controller/`（14）GameController / GameObjectFactory / InputHandler / SceneRouter / EventWiring / SimSystem / screens/
  - `dialog/`（7）DialogLoader / Layout / View / State / Source / Opener / Repository
  - `entities/`（17）Player / NPC / Character / Item / 雨傘家族 / 消耗品 / 拾取物 / DlcSign
  - `gfx/`（6）繪製輔助（drawing helpers，header-only）
  - `quest/`（17）章節任務 / spawn / flags / objective / QuestHookTable
  - `state/`（13）SemesterStateMachine + 各章 State + Interlude + EndingGate（**四結局** Ending_A/B/C/D）
  - `vendor/`（5）攤販 Vendor / Config / Loader / Messages / Sprite
  - `world/`（10）World / Physics / CollisionMask / BuildingTracker / 地形
- **ui/**（21）— 呈現層：`View` 與其下 `hud/`（4）、`overlay/`（3）、`world/`（2），
  外加多個扁平視圖（CharacterSelect / EndingView / InventoryView / MessageView / ChapterCard / RainHud …）。
