---
id: file:src/ui/world/QuestGiverIndicators.cpp
type: source
path: src/ui/world/QuestGiverIndicators.cpp
domain: ui
bucket: world
loc: 45
classes: []
sources: ["src/ui/world/QuestGiverIndicators.cpp"]
---
# `QuestGiverIndicators.cpp`

> **一句定位**：遍歷所有活躍物件，對符合條件者在世界座標繪製任務給予者「!」圖示，是 View 層與 `QuestIndicatorVisible` 決策函式的橋接點。

## 職責

此檔屬於 ui / world 層，只實作 `DrawQuestGiverIndicators` 一個函式。它以 `world.GetPlayer()` 守衛，取出當前章節狀態後，以 `ForEachActive(world.Objects(), ...)` 遍歷所有活躍 `GameObject`，對每一個呼叫 `QuestIndicatorVisible(o.NpcId(), o.IsQuestGiver(), qgState, *player)` 決定是否顯示 `!`，若是則以物件底邊矩形（`Rect{pos.x, pos.y, kPlayerWidth, kPlayerHeight}`）呼叫 `DrawQuestGiverIndicator`。

關鍵設計：所有「!」的顯示判定都透過 `QuestIndicatorVisible` 這個唯一決策點，View 不含任何章節判斷邏輯；第四章終局 NPC 在名冊中 `isQuestGiver=false` 但仍需顯示 `!`，故絕不能以 `o.IsQuestGiver()` 直接短路。函式在 `CameraScope` 內呼叫，使 `!` 圖示疊在建築 / NPC sprite 之上（世界座標）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawQuestGiverIndicators(IRenderer&, World&)` — 唯一公開函式。
- `QuestIndicatorVisible(npcId, isQuestGiver, state, player)` — 決策函式（來自 `QuestIndicator.h`），View 的唯一邏輯依賴。
- `DrawQuestGiverIndicator(r, Rect)` — 來自 `QuestGiverIndicator.h`，繪製單一 `!` 圖示。
- `ForEachActive(world.Objects(), ...)` — 來自 `GameObjectQueries.h`，安全遍歷活躍物件。

## 相依與在架構中的位置

- **#include（往外）**：`QuestGiverIndicators.h`、`GameObjectQueries.h`（`ForEachActive`）、`GameObject.h`、`Player.h`、`QuestIndicator.h`（決策函式）、`SemesterState.h`、`QuestGiverIndicator.h`（繪製）、`World.h`、`WorldConfig.h`（`kPlayerWidth/Height`）、`IRenderer.h`、`Rect.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderWorld` 在 `CameraScope` 內、畫家排序之後呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（world 子層）；在世界座標的畫家排序「之後」繪製，確保 `!` 疊在建築之上；每幀 `RenderWorld` 呼叫。

## OO 概念與設計重點

此函式把「判斷是否顯示 `!`」（邏輯）和「繪製 `!`」（渲染）完全分離：`QuestIndicatorVisible` 封裝了所有章節特殊規則，`DrawQuestGiverIndicator` 只管畫。View 本身完全不含章節判斷，符合 [MVC](../concepts/arch-mvc.md) 的 View 無玩法邏輯原則，也符合 [ISP](../concepts/oo-isp-roles.md)（不用 `dynamic_cast`）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/world/QuestGiverIndicators.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/world/QuestGiverIndicators.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [ISP / Roles](../concepts/oo-isp-roles.md)
