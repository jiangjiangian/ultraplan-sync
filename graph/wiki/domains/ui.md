---
id: domain-ui
type: domain
title: ui
---

# 領域：ui · 視圖層（View）

> 視圖層（MVC 的 View 端）：`View` 主繪圖器，加上 `hud` / `overlay` / `world` 子視圖與扁平視圖（標題、角色選擇、結局卡、說明頁、背包、訊息）。只讀 `const World&`，經 `IRenderer` 輸出。

共 **36** 個檔案，分 4 個 bucket。[在互動圖譜中檢視 →](../../index.html#node=domain:ui)

## ui/(根)  (18)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/ui/ChapterCard.h` | `ChapterCardState` | [node](../../index.html#node=file:include/ui/ChapterCard.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/ChapterCard.h) |
| `include/ui/CharacterSelect.h` | — | [node](../../index.html#node=file:include/ui/CharacterSelect.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/CharacterSelect.h) |
| `include/ui/EndingView.h` | `EndingSummary` | [node](../../index.html#node=file:include/ui/EndingView.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/EndingView.h) |
| `include/ui/GameHelp.h` | — | [node](../../index.html#node=file:include/ui/GameHelp.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/GameHelp.h) |
| `include/ui/HelpPageView.h` | `HelpPageStyle` | [node](../../index.html#node=file:include/ui/HelpPageView.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/HelpPageView.h) |
| `include/ui/InventoryView.h` | — | [node](../../index.html#node=file:include/ui/InventoryView.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/InventoryView.h) |
| `include/ui/MessageView.h` | — | [node](../../index.html#node=file:include/ui/MessageView.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/MessageView.h) |
| `include/ui/PressLatch.h` | `PressLatch` | [node](../../index.html#node=file:include/ui/PressLatch.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/PressLatch.h) |
| `include/ui/QuestGiverIndicator.h` | `QuestGiverIndicatorLayout` | [node](../../index.html#node=file:include/ui/QuestGiverIndicator.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/QuestGiverIndicator.h) |
| `include/ui/RainHud.h` | — | [node](../../index.html#node=file:include/ui/RainHud.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/RainHud.h) |
| `include/ui/ReducedMotion.h` | — | [node](../../index.html#node=file:include/ui/ReducedMotion.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/ReducedMotion.h) |
| `include/ui/View.h` | `View`, `BuildingSprite`, `DrawRef`, `DecorationSprite` | [node](../../index.html#node=file:include/ui/View.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/View.h) |
| `src/ui/ChapterCard.cpp` | — | [node](../../index.html#node=file:src/ui/ChapterCard.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/ChapterCard.cpp) |
| `src/ui/EndingView.cpp` | — | [node](../../index.html#node=file:src/ui/EndingView.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/EndingView.cpp) |
| `src/ui/HelpPageView.cpp` | — | [node](../../index.html#node=file:src/ui/HelpPageView.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/HelpPageView.cpp) |
| `src/ui/InventoryView.cpp` | — | [node](../../index.html#node=file:src/ui/InventoryView.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/InventoryView.cpp) |
| `src/ui/MessageView.cpp` | — | [node](../../index.html#node=file:src/ui/MessageView.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/MessageView.cpp) |
| `src/ui/View.cpp` | — | [node](../../index.html#node=file:src/ui/View.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/View.cpp) |

## ui/hud  (8)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/ui/hud/ObjectiveBar.h` | — | [node](../../index.html#node=file:include/ui/hud/ObjectiveBar.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/ObjectiveBar.h) |
| `include/ui/hud/RainVignette.h` | — | [node](../../index.html#node=file:include/ui/hud/RainVignette.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/RainVignette.h) |
| `include/ui/hud/SportsLapRing.h` | — | [node](../../index.html#node=file:include/ui/hud/SportsLapRing.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/SportsLapRing.h) |
| `include/ui/hud/StatusPanel.h` | — | [node](../../index.html#node=file:include/ui/hud/StatusPanel.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/StatusPanel.h) |
| `src/ui/hud/ObjectiveBar.cpp` | — | [node](../../index.html#node=file:src/ui/hud/ObjectiveBar.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/ObjectiveBar.cpp) |
| `src/ui/hud/RainVignette.cpp` | — | [node](../../index.html#node=file:src/ui/hud/RainVignette.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/RainVignette.cpp) |
| `src/ui/hud/SportsLapRing.cpp` | — | [node](../../index.html#node=file:src/ui/hud/SportsLapRing.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/SportsLapRing.cpp) |
| `src/ui/hud/StatusPanel.cpp` | — | [node](../../index.html#node=file:src/ui/hud/StatusPanel.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/StatusPanel.cpp) |

## ui/overlay  (6)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/ui/overlay/HelpOverlay.h` | — | [node](../../index.html#node=file:include/ui/overlay/HelpOverlay.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/HelpOverlay.h) |
| `include/ui/overlay/MenuAffordance.h` | — | [node](../../index.html#node=file:include/ui/overlay/MenuAffordance.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/MenuAffordance.h) |
| `include/ui/overlay/PauseMenu.h` | — | [node](../../index.html#node=file:include/ui/overlay/PauseMenu.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/PauseMenu.h) |
| `src/ui/overlay/HelpOverlay.cpp` | — | [node](../../index.html#node=file:src/ui/overlay/HelpOverlay.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/HelpOverlay.cpp) |
| `src/ui/overlay/MenuAffordance.cpp` | — | [node](../../index.html#node=file:src/ui/overlay/MenuAffordance.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/MenuAffordance.cpp) |
| `src/ui/overlay/PauseMenu.cpp` | — | [node](../../index.html#node=file:src/ui/overlay/PauseMenu.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/PauseMenu.cpp) |

## ui/world  (4)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/ui/world/QuestGiverIndicators.h` | — | [node](../../index.html#node=file:include/ui/world/QuestGiverIndicators.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/world/QuestGiverIndicators.h) |
| `include/ui/world/SportsLapTrack.h` | — | [node](../../index.html#node=file:include/ui/world/SportsLapTrack.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/world/SportsLapTrack.h) |
| `src/ui/world/QuestGiverIndicators.cpp` | — | [node](../../index.html#node=file:src/ui/world/QuestGiverIndicators.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/world/QuestGiverIndicators.cpp) |
| `src/ui/world/SportsLapTrack.cpp` | — | [node](../../index.html#node=file:src/ui/world/SportsLapTrack.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/world/SportsLapTrack.cpp) |

---
[← wiki 索引](../index.md)