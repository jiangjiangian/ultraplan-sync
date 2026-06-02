---
id: file:src/ui/overlay/MenuAffordance.cpp
type: source
path: src/ui/overlay/MenuAffordance.cpp
domain: ui
bucket: overlay
loc: 52
classes: []
sources: ["src/ui/overlay/MenuAffordance.cpp"]
---
# `MenuAffordance.cpp`

> **一句定位**：HUD 右上角的「M 選單」小標籤——選單未開啟時恆亮顯示，提示玩家有戲內選單可用。

## 職責

此檔屬於 ui / overlay 層，只實作 `DrawMenuAffordance` 一個函式。它以 `world.MenuOpen()` 守衛（選單開啟時直接返回，避免與選單重複），計算 `"M 選單"` 的顯示寬度（以起首位元組計碼點數，取寬鬆估計容納全形字），在螢幕右上角（`screenW - panelW - 6, y=6`）繪製深色半透明底板 + 白色 `TextBuilder{"M 選單"}` 標籤（字體 14pt）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawMenuAffordance(IRenderer&, World&, float W, float /*H*/)` — 唯一公開函式。
- 文字：`"M 選單"` — ASCII + 2 個全形 CJK，字體 14pt。
- 寬度估計：碼點數 × 14px（寬鬆，容納全形）。
- 底板：`Color{20,22,30,170}`；文字：`Colors::White`。

## 相依與在架構中的位置

- **#include（往外）**：`MenuAffordance.h`、`World.h`（`MenuOpen`）、`IRenderer.h`、`TextBuilder.h`、`Color.h`/`Rect.h`/`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderOverlays` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層疊層；在世界 / HUD 之上、選單之前繪製，選單開啟時自動隱藏。

## OO 概念與設計重點

極簡的可尋性（affordance）元件：52 行、無狀態、純讀 Model，是 HUD 設計的最小單元。「選單開啟時隱藏」的守衛確保提示不與選單重疊，而非用額外的可見性旗標管理，符合 YAGNI 精神。符合 [MVC](../concepts/arch-mvc.md) 和 [DIP Renderer](../concepts/arch-dip-renderer.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/overlay/MenuAffordance.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/MenuAffordance.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
