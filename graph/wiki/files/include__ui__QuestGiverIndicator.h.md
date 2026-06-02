---
id: file:include/ui/QuestGiverIndicator.h
type: header
path: include/ui/QuestGiverIndicator.h
domain: ui
bucket: 
loc: 80
classes: [QuestGiverIndicatorLayout]
sources: ["include/ui/QuestGiverIndicator.h"]
---
# `QuestGiverIndicator.h`

> **一句定位**：任務給予者 NPC 頭頂「!」提示的幾何計算（`LayoutQuestGiverIndicator`）與渲染（`DrawQuestGiverIndicator`），header-only 純函式，版面計算可單獨測試。

## 職責

本標頭提供任務給予者 NPC 頭頂金色「!」提示圖示的完整實作。圖示包含：陰影（往右下偏移 2 px 的半透明深色矩形）、16×16 金色方塊（`#FFc83D`）、置中的黑色「!」字形。整個圖示繪製在 `CameraScope` 內，因此跟著 NPC 一起位於世界座標空間，而非螢幕空間。

版面計算被抽出為獨立的 `LayoutQuestGiverIndicator(hitBox) -> QuestGiverIndicatorLayout`，原因是讓測試能在不需模擬渲染器的情況下抽查幾何正確性。計算邏輯：NPC sprite 以碰撞盒底邊對齊（sprite 高 32 px，故頂端在 `hitBox.y + hitBox.height - 32`），圖示再往上抬 `kLiftPx=20` px，圖示水平置中於碰撞盒。「!」字形大小 14，考慮到 raylib 點陣字型的實際像素寬高（約 3×10 px），做了 `(+6, +1)` 的視覺置中微調。

兩個函式均為 `inline`，不需 `.cpp` 單元，不依賴全域狀態，可在無 GL 環境中直接測試。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestGiverIndicatorLayout`（struct）：提示圖示的版面資料：
  - `panel`（`Rect`）：16×16 金色方塊的座標與尺寸（世界座標）。
  - `textPos`（`Vec2`）：「!」字形的左上角座標。
  - `textSize`（`int`）：字體大小（== 14）。
- `LayoutQuestGiverIndicator(hitBox) -> QuestGiverIndicatorLayout`（`[[nodiscard]] inline`）：
  - 常數 `kSize=16`、`kLiftPx=20`、`kSpriteH=32`。
  - 計算 sprite 頂端 `spriteTopY = hitBox.y + hitBox.height - kSpriteH`。
  - 圖示 Y = `spriteTopY - kLiftPx - kSize`；圖示 X = `hitBox.x + (hitBox.width - kSize) * 0.5f`。
  - 文字 = 圖示左上 `(+6, +1)`（視覺置中微調）。
- `DrawQuestGiverIndicator(r, hitBox)`（`inline`）：
  - 呼叫 `LayoutQuestGiverIndicator` 取得版面。
  - `r.DrawRect(shadow)`：陰影（偏移 (+2, +2)，顏色 `{0,0,0,140}`）。
  - `r.DrawRect(panel)`：金色方塊（`{255,200,61,255}`）。
  - `r.DrawText("!", textPos, textSize, Colors::Black)`：黑色「!」字形。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/IRenderer.h`（繪製介面，`DrawRect`、`DrawText`）；`engine/math/Rect.h`（版面矩形型別）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderWorld` 裡在 `CameraScope` 內為有任務旗標的 NPC 呼叫）；`src/ui/world/QuestGiverIndicators.cpp`（任務給予者指示器群組渲染）；`tests/ui/test_quest_giver_indicator.cpp`（版面幾何單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（ui domain），在 `RenderWorld` 階段（`CameraScope` 內，世界座標）繪製，純渲染，不存取 World（hitBox 由呼叫端從 World 提取後傳入）。

## OO 概念與設計重點

`LayoutQuestGiverIndicator` 從 `DrawQuestGiverIndicator` 中抽出版面計算是 **關注點分離** 的典型應用：讓幾何正確性可以被純計算測試驗證，而不需要偽造或 spy 一個渲染器。所有繪製透過 `IRenderer` 注入，符合 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。header-only 設計（兩函式皆 `inline`）讓此工具可被多個消費端直接引用，無需連結額外的 `.cpp`。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/QuestGiverIndicator.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/QuestGiverIndicator.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
