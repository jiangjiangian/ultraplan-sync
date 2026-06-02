---
id: file:src/ui/hud/ObjectiveBar.cpp
type: source
path: src/ui/hud/ObjectiveBar.cpp
domain: ui
bucket: hud
loc: 60
classes: []
sources: ["src/ui/hud/ObjectiveBar.cpp"]
---
# `ObjectiveBar.cpp`

> **一句定位**：HUD 任務目標列——在螢幕頂端置中位置（狀態面板下方）顯示一行當前任務指引，面板寬度隨文字長度自適應。

## 職責

此檔屬於 ui / hud 層，只實作 `DrawObjectiveBar` 一個函式。它讀取 `CurrentObjective(st, *player)` 取得當前章節的目標字串，若字串為空或無玩家則直接返回（不畫面板）。面板寬度以 `TextBuilder{obj}.Size(kObjSize).Measure()` 量測實際文字尺寸後加 padding（kPad=6），確保面板緊貼文字、隨長度伸縮。面板固定在 y=138（位於左上狀態面板最多 6 行高度之下），水平置中（最小 x=4 防出血），以深色半透明背板（`Color{20,22,30,185}`）+ 白色文字（字體 14pt）繪製。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawObjectiveBar(IRenderer&, World&, SemesterState, float W, float /*H*/)` — 唯一公開函式。
- `CurrentObjective(st, player)` — 來自 `QuestObjective.h`，按章節回傳當前目標字串。
- `kObjSize` = 14、`kPad` = 6、`kPanelY` = 138 — 版面常數。
- `TextBuilder::Measure()` — 量測文字像素尺寸（避免猜測固定寬度）。

## 相依與在架構中的位置

- **#include（往外）**：`ObjectiveBar.h`、`Player.h`、`QuestObjective.h`（`CurrentObjective`）、`World.h`（取 `GetPlayer`）、`IRenderer.h`、`TextBuilder.h`、`Color.h`/`Rect.h`/`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderHud` 呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 HUD；每幀 `RenderHud` 中呼叫，只讀 `const World&`，不改寫狀態。

## OO 概念與設計重點

以 `TextBuilder::Measure()` 量測後決定面板寬度，是「響應式 HUD」的實踐（與固定寬度方案相比更能適應不同長度的任務字串）。`CurrentObjective` 的章節路由封裝在 `QuestObjective.h` 中，讓此渲染函式完全不含章節判斷邏輯，符合 SRP。純讀 Model，零副作用，符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/hud/ObjectiveBar.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/ObjectiveBar.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
