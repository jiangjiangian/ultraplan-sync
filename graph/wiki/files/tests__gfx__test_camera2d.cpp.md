---
id: file:tests/gfx/test_camera2d.cpp
type: test
path: tests/gfx/test_camera2d.cpp
domain: tests
bucket: gfx
loc: 43
classes: []
sources: ["tests/gfx/test_camera2d.cpp"]
---
# `test_camera2d.cpp`

> **一句定位**：驗證 `Camera2D` 的預設值、`Follow` 的雙重賦值，以及 `WithZoom` / `WithRotation` 流暢式設定器的串接與回傳 `*this` 語意。

## 職責

此測試以三個 TEST_CASE 覆蓋 `Camera2D` 的基本合約。`Camera2D` 是 engine 層的相機資料型別，封裝 raylib 的對應結構，提供 C++ 友善的 builder 介面。測試驗證：

1. 預設建構後所有欄位的零值語意（offset、target、rotation 均為零，zoom 為 1.0）。
2. `Follow(worldTarget, screenCenter)` 方法將 `target` 設為世界目標座標、`offset` 設為螢幕中心，並回傳 `*this`（支援串接）。
3. `WithZoom()` 和 `WithRotation()` 可串接呼叫，回傳的參考確實是同一個物件（`&ret == &c`）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("Camera2D 預設值：offset/target/rotation 皆為零，zoom 為 1.0")`：斷言六個浮點欄位的初始值。
- `TEST_CASE("Camera2D::Follow 將 target 設為世界目標、offset 設為螢幕中心")`：呼叫 `c.Follow({1000,500}, {400,225})` 後斷言 target 和 offset 的各分量，並驗證回傳 `*this`。
- `TEST_CASE("Camera2D::WithZoom 與 WithRotation 為可串接的流暢式設定器")`：串接 `WithZoom(2.0f).WithRotation(45.0f)` 後斷言兩個欄位與 `&ret == &c`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Camera2D.h`（受測），`engine/math/Vec2.h`（座標）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料結構測試）

## OO 概念與設計重點

流暢介面（Fluent Interface）的串接語意由 `&ret == &c` 斷言嚴格驗證，防止設定器意外回傳副本。這是 C++ builder 模式的基礎正確性測試，確保後續鏈式呼叫不因副本語意而靜默失效。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_camera2d.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_camera2d.cpp) · [← 全檔索引](../files-index.md)
