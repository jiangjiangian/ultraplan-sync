---
id: domain-game
type: domain
title: game
---

# 領域：game · 遊戲邏輯（Model）

> 遊戲邏輯（MVC 的 Model 端）：`entities`（GameObject 家族）、`world`（World / 碰撞）、`state`（學期狀態機 / 結局）、`quest`（鉤子表 / ripple / spawn）、`controller`（GameController / Factory / ISystem 管線）、`dialog`、`vendor`、`gfx`。

共 **139** 個檔案，分 8 個 bucket。[在互動圖譜中檢視 →](../../index.html#node=domain:game)

## game/controller  (26)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/controller/DialogChoiceApply.h` | — | [node](../../index.html#node=file:include/game/controller/DialogChoiceApply.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/DialogChoiceApply.h) |
| `include/game/controller/EventWiring.h` | — | [node](../../index.html#node=file:include/game/controller/EventWiring.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/EventWiring.h) |
| `include/game/controller/GameController.h` | `GameController` | [node](../../index.html#node=file:include/game/controller/GameController.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameController.h) |
| `include/game/controller/GameObjectFactory.h` | `GameObjectFactory` | [node](../../index.html#node=file:include/game/controller/GameObjectFactory.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameObjectFactory.h) |
| `include/game/controller/GameObjectQueries.h` | — | [node](../../index.html#node=file:include/game/controller/GameObjectQueries.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameObjectQueries.h) |
| `include/game/controller/InputHandler.h` | `InputHandler` | [node](../../index.html#node=file:include/game/controller/InputHandler.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/InputHandler.h) |
| `include/game/controller/InteractDispatch.h` | — | [node](../../index.html#node=file:include/game/controller/InteractDispatch.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/InteractDispatch.h) |
| `include/game/controller/SceneRouter.h` | `SceneRouter` | [node](../../index.html#node=file:include/game/controller/SceneRouter.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SceneRouter.h) |
| `include/game/controller/SimSystem.h` | `SimContext`, `ISystem`, `SurvivalSystem`, `MovementSystem`, `CollisionSystem`, `SpawnSystem`, `SweepSystem` | [node](../../index.html#node=file:include/game/controller/SimSystem.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SimSystem.h) |
| `include/game/controller/VendorMenu.h` | — | [node](../../index.html#node=file:include/game/controller/VendorMenu.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/VendorMenu.h) |
| `include/game/controller/screens/DialogScreen.h` | — | [node](../../index.html#node=file:include/game/controller/screens/DialogScreen.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/DialogScreen.h) |
| `include/game/controller/screens/EndingScreen.h` | — | [node](../../index.html#node=file:include/game/controller/screens/EndingScreen.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/EndingScreen.h) |
| `include/game/controller/screens/InventoryScreen.h` | — | [node](../../index.html#node=file:include/game/controller/screens/InventoryScreen.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/InventoryScreen.h) |
| `include/game/controller/screens/PauseScreen.h` | — | [node](../../index.html#node=file:include/game/controller/screens/PauseScreen.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/PauseScreen.h) |
| `src/game/controller/DialogChoiceApply.cpp` | — | [node](../../index.html#node=file:src/game/controller/DialogChoiceApply.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/DialogChoiceApply.cpp) |
| `src/game/controller/GameController.cpp` | — | [node](../../index.html#node=file:src/game/controller/GameController.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/GameController.cpp) |
| `src/game/controller/GameObjectFactory.cpp` | — | [node](../../index.html#node=file:src/game/controller/GameObjectFactory.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/GameObjectFactory.cpp) |
| `src/game/controller/InputHandler.cpp` | — | [node](../../index.html#node=file:src/game/controller/InputHandler.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/InputHandler.cpp) |
| `src/game/controller/InteractDispatch.cpp` | — | [node](../../index.html#node=file:src/game/controller/InteractDispatch.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/InteractDispatch.cpp) |
| `src/game/controller/SceneRouter.cpp` | — | [node](../../index.html#node=file:src/game/controller/SceneRouter.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/SceneRouter.cpp) |
| `src/game/controller/SimSystems.cpp` | — | [node](../../index.html#node=file:src/game/controller/SimSystems.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/SimSystems.cpp) |
| `src/game/controller/VendorMenu.cpp` | — | [node](../../index.html#node=file:src/game/controller/VendorMenu.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/VendorMenu.cpp) |
| `src/game/controller/screens/DialogScreen.cpp` | — | [node](../../index.html#node=file:src/game/controller/screens/DialogScreen.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/DialogScreen.cpp) |
| `src/game/controller/screens/EndingScreen.cpp` | — | [node](../../index.html#node=file:src/game/controller/screens/EndingScreen.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/EndingScreen.cpp) |
| `src/game/controller/screens/InventoryScreen.cpp` | — | [node](../../index.html#node=file:src/game/controller/screens/InventoryScreen.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/InventoryScreen.cpp) |
| `src/game/controller/screens/PauseScreen.cpp` | — | [node](../../index.html#node=file:src/game/controller/screens/PauseScreen.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/PauseScreen.cpp) |

## game/dialog  (13)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/dialog/DialogLayout.h` | — | [node](../../index.html#node=file:include/game/dialog/DialogLayout.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogLayout.h) |
| `include/game/dialog/DialogLoader.h` | `SubEntry`, `LoadedChapter` | [node](../../index.html#node=file:include/game/dialog/DialogLoader.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogLoader.h) |
| `include/game/dialog/DialogOpener.h` | — | [node](../../index.html#node=file:include/game/dialog/DialogOpener.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogOpener.h) |
| `include/game/dialog/DialogRepository.h` | `DialogRepository` | [node](../../index.html#node=file:include/game/dialog/DialogRepository.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogRepository.h) |
| `include/game/dialog/DialogSource.h` | — | [node](../../index.html#node=file:include/game/dialog/DialogSource.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogSource.h) |
| `include/game/dialog/DialogState.h` | `DialogChoice`, `DialogState` | [node](../../index.html#node=file:include/game/dialog/DialogState.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogState.h) |
| `include/game/dialog/DialogView.h` | — | [node](../../index.html#node=file:include/game/dialog/DialogView.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogView.h) |
| `src/game/dialog/DialogLayout.cpp` | `Range` | [node](../../index.html#node=file:src/game/dialog/DialogLayout.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogLayout.cpp) |
| `src/game/dialog/DialogLoader.cpp` | — | [node](../../index.html#node=file:src/game/dialog/DialogLoader.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogLoader.cpp) |
| `src/game/dialog/DialogOpener.cpp` | `DispatchEntry` | [node](../../index.html#node=file:src/game/dialog/DialogOpener.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogOpener.cpp) |
| `src/game/dialog/DialogSource.cpp` | — | [node](../../index.html#node=file:src/game/dialog/DialogSource.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogSource.cpp) |
| `src/game/dialog/DialogState.cpp` | — | [node](../../index.html#node=file:src/game/dialog/DialogState.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogState.cpp) |
| `src/game/dialog/DialogView.cpp` | — | [node](../../index.html#node=file:src/game/dialog/DialogView.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogView.cpp) |

## game/entities  (30)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/entities/CashPickup.h` | `CashPickup` | [node](../../index.html#node=file:include/game/entities/CashPickup.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/CashPickup.h) |
| `include/game/entities/Character.h` | `Character` | [node](../../index.html#node=file:include/game/entities/Character.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Character.h) |
| `include/game/entities/ConsumableItem.h` | `ConsumableItem` | [node](../../index.html#node=file:include/game/entities/ConsumableItem.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/ConsumableItem.h) |
| `include/game/entities/CursedUmbrella.h` | `CursedUmbrella` | [node](../../index.html#node=file:include/game/entities/CursedUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/CursedUmbrella.h) |
| `include/game/entities/DlcSign.h` | `DlcSign` | [node](../../index.html#node=file:include/game/entities/DlcSign.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/DlcSign.h) |
| `include/game/entities/EnergyDrink.h` | `EnergyDrink` | [node](../../index.html#node=file:include/game/entities/EnergyDrink.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/EnergyDrink.h) |
| `include/game/entities/FragileUmbrella.h` | `FragileUmbrella` | [node](../../index.html#node=file:include/game/entities/FragileUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/FragileUmbrella.h) |
| `include/game/entities/HotPack.h` | `HotPack` | [node](../../index.html#node=file:include/game/entities/HotPack.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/HotPack.h) |
| `include/game/entities/Item.h` | `Item` | [node](../../index.html#node=file:include/game/entities/Item.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Item.h) |
| `include/game/entities/NPC.h` | `NPC`, `RenderCell` | [node](../../index.html#node=file:include/game/entities/NPC.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/NPC.h) |
| `include/game/entities/Personas.h` | `Persona`, `CharacterSelectResult` | [node](../../index.html#node=file:include/game/entities/Personas.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Personas.h) |
| `include/game/entities/Player.h` | `Player` | [node](../../index.html#node=file:include/game/entities/Player.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Player.h) |
| `include/game/entities/ProfessorTrapUmbrella.h` | `ProfessorTrapUmbrella` | [node](../../index.html#node=file:include/game/entities/ProfessorTrapUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/ProfessorTrapUmbrella.h) |
| `include/game/entities/QuestFlagPickup.h` | `QuestFlagPickup` | [node](../../index.html#node=file:include/game/entities/QuestFlagPickup.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/QuestFlagPickup.h) |
| `include/game/entities/TransparentUmbrella.h` | `TransparentUmbrella` | [node](../../index.html#node=file:include/game/entities/TransparentUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/TransparentUmbrella.h) |
| `include/game/entities/TrueUmbrella.h` | `TrueUmbrella` | [node](../../index.html#node=file:include/game/entities/TrueUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/TrueUmbrella.h) |
| `include/game/entities/WaterproofSpray.h` | `WaterproofSpray` | [node](../../index.html#node=file:include/game/entities/WaterproofSpray.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/WaterproofSpray.h) |
| `src/game/entities/CashPickup.cpp` | — | [node](../../index.html#node=file:src/game/entities/CashPickup.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/CashPickup.cpp) |
| `src/game/entities/CursedUmbrella.cpp` | — | [node](../../index.html#node=file:src/game/entities/CursedUmbrella.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/CursedUmbrella.cpp) |
| `src/game/entities/DlcSign.cpp` | — | [node](../../index.html#node=file:src/game/entities/DlcSign.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/DlcSign.cpp) |
| `src/game/entities/EnergyDrink.cpp` | — | [node](../../index.html#node=file:src/game/entities/EnergyDrink.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/EnergyDrink.cpp) |
| `src/game/entities/FragileUmbrella.cpp` | — | [node](../../index.html#node=file:src/game/entities/FragileUmbrella.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/FragileUmbrella.cpp) |
| `src/game/entities/HotPack.cpp` | — | [node](../../index.html#node=file:src/game/entities/HotPack.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/HotPack.cpp) |
| `src/game/entities/NPC.cpp` | — | [node](../../index.html#node=file:src/game/entities/NPC.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/NPC.cpp) |
| `src/game/entities/Player.cpp` | — | [node](../../index.html#node=file:src/game/entities/Player.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/Player.cpp) |
| `src/game/entities/ProfessorTrapUmbrella.cpp` | — | [node](../../index.html#node=file:src/game/entities/ProfessorTrapUmbrella.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/ProfessorTrapUmbrella.cpp) |
| `src/game/entities/QuestFlagPickup.cpp` | — | [node](../../index.html#node=file:src/game/entities/QuestFlagPickup.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/QuestFlagPickup.cpp) |
| `src/game/entities/TransparentUmbrella.cpp` | — | [node](../../index.html#node=file:src/game/entities/TransparentUmbrella.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/TransparentUmbrella.cpp) |
| `src/game/entities/TrueUmbrella.cpp` | — | [node](../../index.html#node=file:src/game/entities/TrueUmbrella.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/TrueUmbrella.cpp) |
| `src/game/entities/WaterproofSpray.cpp` | — | [node](../../index.html#node=file:src/game/entities/WaterproofSpray.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/WaterproofSpray.cpp) |

## game/gfx  (6)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/gfx/Bounds.h` | — | [node](../../index.html#node=file:include/game/gfx/Bounds.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/Bounds.h) |
| `include/game/gfx/Decorations.h` | — | [node](../../index.html#node=file:include/game/gfx/Decorations.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/Decorations.h) |
| `include/game/gfx/MaskLoader.h` | — | [node](../../index.html#node=file:include/game/gfx/MaskLoader.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/MaskLoader.h) |
| `include/game/gfx/SpriteStrip.h` | `DecorationDef` | [node](../../index.html#node=file:include/game/gfx/SpriteStrip.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/SpriteStrip.h) |
| `include/game/gfx/UmbrellaGlyph.h` | — | [node](../../index.html#node=file:include/game/gfx/UmbrellaGlyph.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/UmbrellaGlyph.h) |
| `include/game/gfx/WalkCycle.h` | — | [node](../../index.html#node=file:include/game/gfx/WalkCycle.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/WalkCycle.h) |

## game/quest  (26)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/quest/Chapter1Quest.h` | — | [node](../../index.html#node=file:include/game/quest/Chapter1Quest.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter1Quest.h) |
| `include/game/quest/Chapter2Quest.h` | — | [node](../../index.html#node=file:include/game/quest/Chapter2Quest.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter2Quest.h) |
| `include/game/quest/Chapter3Quest.h` | — | [node](../../index.html#node=file:include/game/quest/Chapter3Quest.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter3Quest.h) |
| `include/game/quest/Chapter4Quest.h` | — | [node](../../index.html#node=file:include/game/quest/Chapter4Quest.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Chapter4Quest.h) |
| `include/game/quest/ChapterGate.h` | — | [node](../../index.html#node=file:include/game/quest/ChapterGate.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterGate.h) |
| `include/game/quest/ChapterPickups.h` | `PickupPlacement` | [node](../../index.html#node=file:include/game/quest/ChapterPickups.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterPickups.h) |
| `include/game/quest/ChapterQuestItems.h` | `QuestItemPlacement` | [node](../../index.html#node=file:include/game/quest/ChapterQuestItems.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterQuestItems.h) |
| `include/game/quest/ChapterSpawns.h` | — | [node](../../index.html#node=file:include/game/quest/ChapterSpawns.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterSpawns.h) |
| `include/game/quest/ChapterVendors.h` | `VendorPlacement` | [node](../../index.html#node=file:include/game/quest/ChapterVendors.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterVendors.h) |
| `include/game/quest/Flags.h` | — | [node](../../index.html#node=file:include/game/quest/Flags.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/Flags.h) |
| `include/game/quest/InventoryPaging.h` | — | [node](../../index.html#node=file:include/game/quest/InventoryPaging.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/InventoryPaging.h) |
| `include/game/quest/ItemCatalog.h` | `ItemInfo`, `InventoryRow` | [node](../../index.html#node=file:include/game/quest/ItemCatalog.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ItemCatalog.h) |
| `include/game/quest/NpcSpawns.h` | `NpcSpawn` | [node](../../index.html#node=file:include/game/quest/NpcSpawns.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/NpcSpawns.h) |
| `include/game/quest/PipoyaRoster.h` | — | [node](../../index.html#node=file:include/game/quest/PipoyaRoster.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/PipoyaRoster.h) |
| `include/game/quest/QuestHookTable.h` | `QuestHook` | [node](../../index.html#node=file:include/game/quest/QuestHookTable.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestHookTable.h) |
| `include/game/quest/QuestIndicator.h` | — | [node](../../index.html#node=file:include/game/quest/QuestIndicator.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestIndicator.h) |
| `include/game/quest/QuestObjective.h` | — | [node](../../index.html#node=file:include/game/quest/QuestObjective.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestObjective.h) |
| `src/game/quest/Chapter1Quest.cpp` | — | [node](../../index.html#node=file:src/game/quest/Chapter1Quest.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter1Quest.cpp) |
| `src/game/quest/Chapter2Quest.cpp` | — | [node](../../index.html#node=file:src/game/quest/Chapter2Quest.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter2Quest.cpp) |
| `src/game/quest/Chapter3Quest.cpp` | — | [node](../../index.html#node=file:src/game/quest/Chapter3Quest.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter3Quest.cpp) |
| `src/game/quest/Chapter4Quest.cpp` | — | [node](../../index.html#node=file:src/game/quest/Chapter4Quest.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter4Quest.cpp) |
| `src/game/quest/ChapterGate.cpp` | — | [node](../../index.html#node=file:src/game/quest/ChapterGate.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ChapterGate.cpp) |
| `src/game/quest/ChapterVendors.cpp` | — | [node](../../index.html#node=file:src/game/quest/ChapterVendors.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ChapterVendors.cpp) |
| `src/game/quest/ItemCatalog.cpp` | — | [node](../../index.html#node=file:src/game/quest/ItemCatalog.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ItemCatalog.cpp) |
| `src/game/quest/QuestHookTable.cpp` | — | [node](../../index.html#node=file:src/game/quest/QuestHookTable.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/QuestHookTable.cpp) |
| `src/game/quest/QuestIndicator.cpp` | — | [node](../../index.html#node=file:src/game/quest/QuestIndicator.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/QuestIndicator.cpp) |

## game/state  (15)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/state/Chapter1AddDrop.h` | `Chapter1AddDrop` | [node](../../index.html#node=file:include/game/state/Chapter1AddDrop.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter1AddDrop.h) |
| `include/game/state/Chapter2Midterms.h` | `Chapter2Midterms` | [node](../../index.html#node=file:include/game/state/Chapter2Midterms.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter2Midterms.h) |
| `include/game/state/Chapter3SportsDay.h` | `Chapter3SportsDay` | [node](../../index.html#node=file:include/game/state/Chapter3SportsDay.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter3SportsDay.h) |
| `include/game/state/Chapter4Finals.h` | `Chapter4Finals` | [node](../../index.html#node=file:include/game/state/Chapter4Finals.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/Chapter4Finals.h) |
| `include/game/state/ChapterToast.h` | — | [node](../../index.html#node=file:include/game/state/ChapterToast.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/ChapterToast.h) |
| `include/game/state/EndingGate.h` | — | [node](../../index.html#node=file:include/game/state/EndingGate.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/EndingGate.h) |
| `include/game/state/EndingMenuModel.h` | — | [node](../../index.html#node=file:include/game/state/EndingMenuModel.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/EndingMenuModel.h) |
| `include/game/state/GameHelpPages.h` | — | [node](../../index.html#node=file:include/game/state/GameHelpPages.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/GameHelpPages.h) |
| `include/game/state/InterludeExit.h` | — | [node](../../index.html#node=file:include/game/state/InterludeExit.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeExit.h) |
| `include/game/state/InterludeExitMarker.h` | `InterludeExitMarkerDash`, `InterludeExitMarkerLayout` | [node](../../index.html#node=file:include/game/state/InterludeExitMarker.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeExitMarker.h) |
| `include/game/state/InterludeMarket.h` | `InterludeMarket` | [node](../../index.html#node=file:include/game/state/InterludeMarket.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeMarket.h) |
| `include/game/state/SemesterState.h` | `IChapterState` | [node](../../index.html#node=file:include/game/state/SemesterState.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterState.h) |
| `include/game/state/SemesterStateMachine.h` | `SemesterStateMachine` | [node](../../index.html#node=file:include/game/state/SemesterStateMachine.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/SemesterStateMachine.h) |
| `src/game/state/EndingGate.cpp` | — | [node](../../index.html#node=file:src/game/state/EndingGate.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/state/EndingGate.cpp) |
| `src/game/state/SemesterStateMachine.cpp` | — | [node](../../index.html#node=file:src/game/state/SemesterStateMachine.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/state/SemesterStateMachine.cpp) |

## game/vendor  (7)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/vendor/Vendor.h` | `Vendor` | [node](../../index.html#node=file:include/game/vendor/Vendor.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/Vendor.h) |
| `include/game/vendor/VendorConfig.h` | `VendorItem`, `VendorConfig` | [node](../../index.html#node=file:include/game/vendor/VendorConfig.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorConfig.h) |
| `include/game/vendor/VendorLoader.h` | — | [node](../../index.html#node=file:include/game/vendor/VendorLoader.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorLoader.h) |
| `include/game/vendor/VendorMessages.h` | — | [node](../../index.html#node=file:include/game/vendor/VendorMessages.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorMessages.h) |
| `include/game/vendor/VendorSprite.h` | — | [node](../../index.html#node=file:include/game/vendor/VendorSprite.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorSprite.h) |
| `src/game/vendor/Vendor.cpp` | — | [node](../../index.html#node=file:src/game/vendor/Vendor.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/vendor/Vendor.cpp) |
| `src/game/vendor/VendorLoader.cpp` | — | [node](../../index.html#node=file:src/game/vendor/VendorLoader.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/vendor/VendorLoader.cpp) |

## game/world  (16)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/world/BuildingTracker.h` | `BuildingTracker` | [node](../../index.html#node=file:include/game/world/BuildingTracker.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/BuildingTracker.h) |
| `include/game/world/Buildings.h` | `Building` | [node](../../index.html#node=file:include/game/world/Buildings.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Buildings.h) |
| `include/game/world/CollisionMask.h` | `CollisionMask` | [node](../../index.html#node=file:include/game/world/CollisionMask.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/CollisionMask.h) |
| `include/game/world/HudTiming.h` | — | [node](../../index.html#node=file:include/game/world/HudTiming.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/HudTiming.h) |
| `include/game/world/Obstacles.h` | — | [node](../../index.html#node=file:include/game/world/Obstacles.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Obstacles.h) |
| `include/game/world/Physics.h` | — | [node](../../index.html#node=file:include/game/world/Physics.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Physics.h) |
| `include/game/world/TexturePreload.h` | — | [node](../../index.html#node=file:include/game/world/TexturePreload.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/TexturePreload.h) |
| `include/game/world/World.h` | `World` | [node](../../index.html#node=file:include/game/world/World.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/World.h) |
| `include/game/world/WorldConfig.h` | — | [node](../../index.html#node=file:include/game/world/WorldConfig.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/WorldConfig.h) |
| `include/game/world/WorldOptions.h` | `WorldOptions` | [node](../../index.html#node=file:include/game/world/WorldOptions.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/WorldOptions.h) |
| `src/game/world/BuildingTracker.cpp` | — | [node](../../index.html#node=file:src/game/world/BuildingTracker.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/BuildingTracker.cpp) |
| `src/game/world/TerrainMask.cpp` | — | [node](../../index.html#node=file:src/game/world/TerrainMask.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/TerrainMask.cpp) |
| `src/game/world/World.cpp` | — | [node](../../index.html#node=file:src/game/world/World.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/World.cpp) |
| `src/game/world/WorldOptions.cpp` | — | [node](../../index.html#node=file:src/game/world/WorldOptions.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldOptions.cpp) |
| `src/game/world/WorldSpawn.cpp` | — | [node](../../index.html#node=file:src/game/world/WorldSpawn.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldSpawn.cpp) |
| `src/game/world/WorldSportsLap.cpp` | — | [node](../../index.html#node=file:src/game/world/WorldSportsLap.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldSportsLap.cpp) |

---
[← wiki 索引](../index.md)