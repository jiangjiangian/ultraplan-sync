---
id: "file:include/ui/world/QuestGiverIndicators.h"
type: header
path: include/ui/world/QuestGiverIndicators.h
domain: ui
bucket: world
loc: 30
classes: []
sources: ["include/ui/world/QuestGiverIndicators.h"]
---
# `QuestGiverIndicators.h`

> **一句定位**：宣告在任務給予者 NPC 上方繪製「!」浮標提示的純渲染自由函式。

## 職責

此標頭宣告 `DrawQuestGiverIndicators` 自由函式，走訪 `World` 中所有存活的 NPC，對每個 `QuestIndicatorVisible(npcId, IsQuestGiver, semester, player)` 判定為真者，在其頭上渲染世界座標中的任務「!」浮標。

呼叫時機位於繪製順序掃描「之後」、仍在呼叫端持有的 `CameraScope`「之內」，使浮標在世界座標跟隨 NPC，並疊在建築 / sprite 之上（確保躲在建築輪廓後的任務給予者也能被玩家看到）。

`QuestIndicatorVisible` 是唯一的判定點，將 `IsQuestGiver()` 位元與各章規則整合在一起，使 View 維持純渲染（無遊戲邏輯、無 `dynamic_cast`）。第四章終局 NPC 的 `isQuestGiver=false`，故此判定不能僅靠 `IsQuestGiver()` 短路，由 `QuestIndicatorVisible` 負責。無玩家時為空操作，每幀呼叫皆安全。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawQuestGiverIndicators(IRenderer& r, const World& world)` — 走訪存活 NPC，依 `QuestIndicatorVisible` 篩選後，在通過判定的 NPC 上方繪製「!」浮標；在 CameraScope 內執行以取得世界座標。

## 相依與在架構中的位置
- **#include（往外）**：僅依賴前向宣告 `World` 與 `IRenderer`（無實際 include 解析）
- **被誰使用（往內）**：`src/ui/View.cpp`、`src/ui/world/QuestGiverIndicators.cpp`
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；於 `RenderWorld` 的後期（掃描之後）呼叫，在 CameraScope 內，純渲染

## OO 概念與設計重點

此函式將「誰有任務指示符」的判定集中於 `QuestIndicatorVisible`，是 SRP 與 DRY 原則的體現：View 只負責繪製，判斷邏輯不散落在此。[DIP](../concepts/arch-dip-renderer.md) 透過 `IRenderer&` 完成；無 `dynamic_cast` 依賴符合 [ISP / Roles](../concepts/oo-isp-roles.md) 精神。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/ui/world/QuestGiverIndicators.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/world/QuestGiverIndicators.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
