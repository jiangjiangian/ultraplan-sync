---
id: file:include/engine/render/Camera2D.h
type: header
path: include/engine/render/Camera2D.h
domain: engine
bucket: render
loc: 64
classes: [Camera2D]
sources: ["include/engine/render/Camera2D.h"]
---
# `Camera2D.h`

> **一句定位**：raylib `::Camera2D` 的引擎層轉接型別，以引擎 Vec2 描述 2D 攝影機參數，並提供 Follow / WithZoom / ClampToWorld 流暢設定介面。

## 職責

`Camera2D` 是 `::Camera2D` 的純資料轉接器：欄位 `offset`（螢幕偏移）、`target`（世界座標）、`rotation`、`zoom` 與 raylib 的 `::Camera2D` 一一對映，使 `CameraScope`（`BeginMode2D` 的 RAII 守衛）在建構時做欄位對映即可，上層程式碼（View）不需引入 raylib。

三個流暢設定方法，均回傳 `*this` 支援鏈式呼叫：
- `Follow(Vec2 worldTarget, Vec2 screenCenter)`：讓攝影機追隨某世界座標，偏移對齊螢幕中心——俯視角 RPG 追蹤玩家的標準設定。
- `WithZoom(float z)`：設定縮放倍率。
- `ClampToWorld(Vec2 worldSize, Vec2 viewportSize)`：夾住 `target`，使視口不超出世界邊界；若世界比視口還小，把 `target` 釘在世界中點（防止顯示世界外的空白）。`ClampToWorld` 對每個軸獨立判斷，使橫向 / 縱向不對稱的世界能正確處理。

`Camera2D` 在 `View` 中被用來計算追蹤玩家後的攝影機參數，並傳給 `CameraScope`。測試 `test_camera2d / test_camera2d_clamp` 直接測試此型別的數值行為。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::render::Camera2D`**（struct）：`offset`（Vec2）、`target`（Vec2）、`rotation`（float）、`zoom`（float，預設 1.0）。
  - `Follow(Vec2 worldTarget, Vec2 screenCenter) noexcept → Camera2D&`：追蹤目標 + 對齊螢幕中心。
  - `WithZoom(float z) noexcept → Camera2D&`：設定縮放。
  - `WithRotation(float r) noexcept → Camera2D&`：設定旋轉。
  - `ClampToWorld(Vec2 worldSize, Vec2 viewportSize) noexcept → Camera2D&`：夾住 target 使視口不出界；世界 < 視口時釘在中點。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`（`offset / target` 欄位型別）——不依賴 raylib，維持上層程式碼無需引入 raylib。
- **被誰使用（往內）**：`include/engine/render/CameraScope.h`（建構 `::Camera2D` 時做欄位對映）、`include/ui/View.h`（持有 Camera2D 並驅動追蹤邏輯）；測試 `test_camera2d / test_camera2d_clamp`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/render 層；在 `View::Draw` 中每幀更新（`Follow + ClampToWorld`），傳給 `CameraScope` 包覆世界繪製。

## OO 概念與設計重點

`Camera2D` 是「Adapter 模式」的值型別版本：適配 raylib 的 `::Camera2D`，讓上層以引擎型別（`Vec2`）操作攝影機，在 `CameraScope` 邊界才轉回 raylib 格式。流暢 API（`Follow().WithZoom().ClampToWorld()`）以方法鏈取代建構子長參數列，增強可讀性。`ClampToWorld` 的邊界夾算法是俯視角 RPG 的標準技術，測試 `test_camera2d_clamp` 驗證其數值正確性。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/Camera2D.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Camera2D.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP：IRenderer](../concepts/arch-dip-renderer.md)
