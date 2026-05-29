# 檔案組織 — 10-bucket 分層（已實作）

## TL;DR

- `include/`、`src/`、`tests/` 各分成 **10 個對齊的子資料夾**：
  `gfx/ entities/ world/ quest/ state/ dialog/ vendor/ ui/ controller/ harness/`
- 每個 bucket 在三邊都有 `README.md`，列出責任、檔案、依賴方向。
- `src/main.cpp` 仍保留在 `src/` 根（composition root，CLAUDE.md §5）。
- `CMakeLists.txt` 已是 `GLOB_RECURSE`（`src/` 與 `tests/`）所以**無**需手列檔案。
- 每個 `#include "Foo.h"` 一律寫成 `#include "<bucket>/Foo.h"`。`gfx/` 早就是這樣，其他 bucket 在 Cycle 11 對齊完成。

---

## 為什麼是 10 桶而非當初提案 A 的 6 桶？

`docs/FILE_ORGANIZATION.md` 的歷史版本提案 A 是 6 桶（`entities/ world/ state/ event/ factory/ ui/`）。從 Cycle 1 到 10 之間又長出了：

- `quest/` — 章節 quest 控制器 + spawn / pickup / vendor table（提案 A 沒有）
- `dialog/` — runtime dialogue 載入 / 排版 / 播放（原本散在多處）
- `vendor/` — 商人從一般 NPC 拆出，因為它擁有庫存 / 價格 / 拒絕對白
- `controller/` — 不只 EventBus + Factory，還包含 GameController / InputHandler / SceneRouter（Cycle 10.P0a 拆分）
- `harness/` — autoplay 框架（CLAUDE.md §4），與 controller 分桶以保 normal-play bit-for-bit

`gfx/` 一直是子資料夾，這次無變動。`event/` 與 `factory/` 併入 `controller/`，避免 1-2 檔的小桶。

---

## 實作的目錄樹

```
include/
├── gfx/         RAII / value-type wrappers over raylib (lowest layer)
├── entities/    GameObject 階層：Player, NPC, Item, 五種 Umbrella, Consumable 群, Pickup 群
├── world/       World 容器、Buildings、Obstacles、Physics、CollisionMask、BuildingTracker
├── quest/       Chapter{2,3,4}Quest、ChapterGate、Chapter{Pickups,QuestItems,Spawns,Vendors}、
│                NpcSpawns、PipoyaRoster、QuestObjective
├── state/       SemesterStateMachine + Chapter1AddDrop / Chapter2Midterms / Chapter3SportsDay /
│                Chapter4Finals / Interlude{Market,Exit,ExitMarker} / EndingGate
├── dialog/      DialogLoader、DialogSource、DialogState、DialogLayout、DialogView、DialogOpener
├── vendor/      Vendor、VendorConfig、VendorLoader、VendorMessages、VendorSprite
├── ui/          View、TitleScreen、CharacterSelect、EndingView、InventoryView、MessageView、
│                RainHud、HudSlot、ChapterToast、QuestGiverIndicator、GameHelp、ReducedMotion
├── controller/  GameController、InputHandler、SceneRouter、EventBus、EventWiring、
│                GameObjectFactory、GameObjectQueries
└── harness/     Harness、ScriptInput
src/
├── main.cpp     (composition root — stays at root, CLAUDE.md §5)
├── gfx/         (empty — gfx headers are header-only RAII wrappers)
├── entities/    (12 .cpp — Player、NPC、Pickup 群、Umbrella 群、Consumable 群)
├── world/       World、TerrainMask、BuildingTracker
├── quest/       Chapter{2,3,4}Quest、ChapterGate、ChapterVendors (其餘 quest header-only)
├── state/       SemesterStateMachine、EndingGate (其餘 chapter / interlude header-only)
├── dialog/      6 個對應 dialog/ header 的 impl
├── vendor/      Vendor、VendorLoader
├── ui/          View、TitleScreen、CharacterSelect、EndingView、InventoryView、MessageView
├── controller/  GameController、InputHandler、SceneRouter、EventBus、GameObjectFactory
└── harness/     Harness、ScriptInput
tests/
├── gfx/         7 個 pure-value POD wrapper tests
├── entities/    11 個 Player / NPC / Item / Pickup / Umbrella 行為 tests
├── world/       4 個 World / Physics / CollisionMask / BuildingTracker tests
├── quest/       23 個（最大 bucket）— chapter spine、ripple、roster、economy、gate
├── state/       5 個 state machine / interlude / ending gate tests
├── dialog/      8 個 dialog parser / layout / source / state tests
├── vendor/      5 個 vendor actor / loader / inventory tests
├── ui/          15 個 HUD / view / a11y tests
├── controller/  8 個 input / scene router / event bus / factory tests
├── harness/     3 個 ScriptInput tests
└── fixtures/    test-only data assets (no .cpp)
```

每個 bucket 詳細責任 + 依賴方向見：

- include/: [gfx](../include/gfx/README.md) · [entities](../include/entities/README.md) · [world](../include/world/README.md) · [quest](../include/quest/README.md) · [state](../include/state/README.md) · [dialog](../include/dialog/README.md) · [vendor](../include/vendor/README.md) · [ui](../include/ui/README.md) · [controller](../include/controller/README.md) · [harness](../include/harness/README.md)
- src/: [gfx](../src/gfx/README.md) · [entities](../src/entities/README.md) · [world](../src/world/README.md) · [quest](../src/quest/README.md) · [state](../src/state/README.md) · [dialog](../src/dialog/README.md) · [vendor](../src/vendor/README.md) · [ui](../src/ui/README.md) · [controller](../src/controller/README.md) · [harness](../src/harness/README.md)
- tests/: [gfx](../tests/gfx/README.md) · [entities](../tests/entities/README.md) · [world](../tests/world/README.md) · [quest](../tests/quest/README.md) · [state](../tests/state/README.md) · [dialog](../tests/dialog/README.md) · [vendor](../tests/vendor/README.md) · [ui](../tests/ui/README.md) · [controller](../tests/controller/README.md) · [harness](../tests/harness/README.md)

---

## CMakeLists.txt：唯一改動

`tests/` 的 glob 由 `GLOB` 改為 `GLOB_RECURSE`，以便撈到 `tests/<bucket>/*.cpp`。`src/` 的 glob 早就是 `GLOB_RECURSE`。

```cmake
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
```

`target_include_directories(... PUBLIC "${INCLUDE_DIR}")` 只有一個 include root，所以 `#include "<bucket>/Foo.h"` 直接 resolve 不需要增 include path。

---

## #include 規範

- 所有 project header：`#include "<bucket>/Foo.h"`（永遠帶 bucket 前綴）。
- 第三方：`#include "raylib.h"`、`#include "doctest/doctest.h"` 不變。
- 系統 header：`#include <...>` 不變。

`include/<bucket>/Foo.h` 內部互引可以省略前綴（同層）但目前一律保留前綴，掃起來最一致。

---

## 注意事項

- **gfx/** 是 header-only 的 RAII / value-type wrappers，所以 `src/gfx/` 是空的（保留資料夾以維持對稱；要加 `.cpp` 立刻能被 `GLOB_RECURSE` 撈到）。
- **state/** 與 **quest/** 也有不少 header-only chapter / table — `src/state/` 只有 2 個 `.cpp`，`src/quest/` 只有 5 個。
- **harness/** 是 top-of-the-chain：只有 `main.cpp` 引用它；它的 sources 全部隔在 `UMBRELLA_SCRIPT` 後面，normal-play 完全 bypass。
- **resources/** 維持不動（CLAUDE.md §5）。
- **docs/content/** 維持不動（dialogue 是 runtime-loaded — 見 CLAUDE.md §6）。
