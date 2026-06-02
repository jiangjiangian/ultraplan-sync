---
id: file:tests/gfx/test_camera2d_clamp.cpp
type: test
path: tests/gfx/test_camera2d_clamp.cpp
domain: tests
bucket: gfx
loc: 56
classes: []
sources: ["tests/gfx/test_camera2d_clamp.cpp"]
---
# `test_camera2d_clamp.cpp`

> **一句定位**：驗證 `Camera2D::ClampToWorld` 的視口邊界夾限語意——確保相機 target 的選定使視口不超出世界邊界，包含世界比視口小時的置中特例。

## 職責

此測試以五個 TEST_CASE 完整覆蓋 `ClampToWorld(worldSize, screenSize)` 的所有幾何情況。夾限的語意是：`target` 必須讓「以 target 為中心、以 offset 為螢幕錨點的視口」完整落在 `[0, worldSize]` 內；等價地，`target` 要在 `[offset, worldSize - (screenSize - offset)]` 之間。

當世界尺寸小於視口時（特殊情況），夾限至 `worldSize / 2`（置中）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ClampToWorld：target 接近世界中央時不被夾限")`：`Follow({1000,1000}, {400,225})`，世界 `(2048,2048)`，視口 `(800,450)`；target 不變。
- `TEST_CASE("ClampToWorld：target 在世界原點時夾限至半個視口處")`：`Follow({0,0}, {400,225})`；夾限後 `target = (400, 225)`（即 offset 值）。
- `TEST_CASE("ClampToWorld：target 超出右下角時夾限至「世界 - 半視口」處")`：`Follow({5000,5000},...)`；夾限後 `target = (2048-400, 2048-225) = (1648, 1823)`。
- `TEST_CASE("ClampToWorld：世界比視口還小時夾限至世界中點")`：世界 `(200,100)`，視口 `(800,450)`；夾限後 `target = (100, 50)` = 世界各軸 / 2。
- `TEST_CASE("ClampToWorld 回傳 *this 以支援流暢式串接")`：串接 `Follow(...).ClampToWorld(...)` 並驗證 `&ret == &c`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Camera2D.h`（受測），`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純相機邏輯測試）

## OO 概念與設計重點

完整的邊界值分析：正常中央、左上邊界、右下邊界、世界小於視口的退化情況，以及流暢介面串接正確性。`ClampToWorld` 是無狀態（除修改 `target` 外）的純計算方法，這使其易於測試。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_camera2d_clamp.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_camera2d_clamp.cpp) · [← 全檔索引](../files-index.md)
