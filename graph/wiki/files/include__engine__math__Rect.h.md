---
id: file:include/engine/math/Rect.h
type: header
path: include/engine/math/Rect.h
domain: engine
bucket: math
loc: 49
classes: [Rect]
sources: ["include/engine/math/Rect.h"]
---
# `Rect.h`

> **一句定位**：軸對齊矩形（AABB）基礎型別，提供 Contains 和 Intersects 測試，是全專案碰撞偵測與 UI 佈局的幾何核心。

## 職責

`Rect` 是一個 aggregate struct，以「左上角座標 + 寬高」描述軸對齊矩形：`x`、`y`（左上角）、`width`、`height`（向右下延伸），所有成員均為 `float`，預設值 `0`。

提供兩個 `constexpr` 幾何方法：
- `Contains(Vec2 p) const noexcept → bool`：半開區間點測試（右/下邊界不含），避免相鄰矩形重複命中——使用 `p.x >= x && p.x < x + width && p.y >= y && p.y < y + height` 的標準 AABB 點包含條件。
- `Intersects(Rect o) const noexcept → bool`：AABB 重疊測試（分離軸判斷），兩矩形有重疊時回傳 true。

全標頭 header-only，`constexpr` 實作，零執行期成本。`Rect` 在這個程式中扮演雙重角色：碰撞盒（`GameObject::hitBox_` 型別）和 UI 佈局矩形（渲染器的 `DrawRect / DrawSprite` 參數型別）。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::math::Rect`**（struct，aggregate）：`x, y, width, height`（`float`，預設 0）。
  - `Contains(Vec2 p) const noexcept → bool`（`constexpr`）：半開區間點包含測試。
  - `Intersects(Rect o) const noexcept → bool`（`constexpr`）：AABB 矩形重疊測試（分離軸方法）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`（`Contains` 以 `Vec2` 表示點）。
- **被誰使用（往內）**：`include/engine/core/GameObject.h`（`hitBox_` 型別）、`include/engine/render/IRenderer.h` / `Renderer.h`（繪圖函式參數）、`include/game/controller/GameController.h`、`include/game/controller/SimSystem.h`、`include/game/world/Physics.h`、多個實體標頭（`NPC / SpriteStrip / UmbrellaGlyph / Buildings`）、UI 標頭（`View / HelpPageView / QuestGiverIndicator`）及大量 `.cpp`。測試 `tests/gfx/test_rect.cpp` 直接測試此型別。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/math 層；在碰撞管線（Collision 階段）作為碰撞盒型別，在渲染管線作為目標矩形型別。

## OO 概念與設計重點

純資料型別（aggregate），全 `constexpr`，header-only，零依賴（只引入 `Vec2.h`）。`Contains` 的半開區間設計是計算幾何的標準實踐（防止邊界重複命中）。`Intersects` 的分離軸判斷是最高效的 AABB 碰撞實作（四個不等式，短路求值）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/math/Rect.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Rect.h) · [← 全檔索引](../files-index.md)
