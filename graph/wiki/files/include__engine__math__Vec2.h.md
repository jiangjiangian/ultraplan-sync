---
id: file:include/engine/math/Vec2.h
type: header
path: include/engine/math/Vec2.h
domain: engine
bucket: math
loc: 48
classes: [Vec2]
sources: ["include/engine/math/Vec2.h"]
---
# `Vec2.h`

> **一句定位**：二維浮點向量基礎型別，提供位置、速度、尺寸的最小值型別，含長度、正規化與算術運算符，不依賴 raylib。

## 職責

`Vec2` 是 aggregate struct，含兩個 `float` 成員 `x`、`y`（預設 0）。這是整個引擎最底層的幾何原語，被幾乎所有需要 2D 座標或方向的型別依賴。

提供的方法：
- `Length() const noexcept → float`：計算歐氏範數（`sqrt(x²+y²)`）。
- `Normalized() const noexcept → Vec2`：回傳單位向量；長度小於 1e-6 時回傳零向量，防止除以 0。

自由函式（均 `constexpr`）：`operator+`、`operator-`（逐分量加減）、`operator*(Vec2, float)`、`operator*(float, Vec2)`（向量乘以純量，含交換律重載）。

不依賴 raylib。在渲染層，`RaylibRenderer` / `CameraScope` 等在呼叫 raylib API 時才做 `Vec2 → ::Vector2` 的欄位對映（`Vector2{v.x, v.y}`），上層程式碼全程只見 `Vec2`。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::math::Vec2`**（struct，aggregate）：`x, y`（`float`，預設 0）。
  - `Length() const noexcept → float`：`std::sqrt(x*x + y*y)`。
  - `Normalized() const noexcept → Vec2`：單位向量；零向量保護。
- **`operator+(Vec2, Vec2) noexcept → Vec2`**（`constexpr`）：逐分量相加。
- **`operator-(Vec2, Vec2) noexcept → Vec2`**（`constexpr`）：逐分量相減。
- **`operator*(Vec2, float) noexcept → Vec2`**（`constexpr`）：向量乘純量。
- **`operator*(float, Vec2) noexcept → Vec2`**（`constexpr`）：純量乘向量（交換律）。

## 相依與在架構中的位置

- **#include（往外）**：`<cmath>`（`std::sqrt`）——不依賴任何引擎或外部庫標頭。
- **被誰使用（往內）**：`include/engine/math/Rect.h`（`Contains` 參數型別）、`include/engine/core/GameObject.h`（`position_` 型別）、`include/engine/render/Camera2D.h`、`include/engine/render/IRenderer.h`、幾乎所有遊戲實體標頭（Character / Player / DlcSign / CashPickup / QuestFlagPickup）、World / Physics / CollisionMask / Spawn / Vendor / View 等及大量測試。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/math 層最底層型別；在每幀管線的 Movement（位置更新）、Collision（碰撞盒計算）、Spawn（物件生成位置）中均以 `Vec2` 傳遞座標資訊。

## OO 概念與設計重點

純資料型別（aggregate）+ `constexpr` 運算符，header-only，零副作用。`Normalized()` 的零向量保護（epsilon 判斷 `1e-6f`）是實務上防止 NaN 的標準手法。全 `constexpr` 算術讓碰撞盒計算、物件生成座標等能在 compile-time 評估（若使用 constexpr context）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/math/Vec2.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Vec2.h) · [← 全檔索引](../files-index.md)
