---
id: file:include/engine/math/Color.h
type: header
path: include/engine/math/Color.h
domain: engine
bucket: math
loc: 58
classes: [Color]
sources: ["include/engine/math/Color.h"]
---
# `Color.h`

> **一句定位**：RGBA8 顏色值型別與常用顏色常數，不依賴 raylib，作為引擎內部的顏色載體。

## 職責

`Color.h` 定義 `nccu::engine::math::Color`——一個每通道 8 位元的 RGBA 顏色 struct（`r, g, b, a` 各為 `uint8_t`，預設不透明黑色 `{0, 0, 0, 255}`）。

提供 `constexpr WithAlpha(uint8_t newA)` 方法，回傳 RGB 不變、僅替換 Alpha 的副本，方便在 UI 繪製半透明效果而無需手動複製。

`constexpr operator==` 和 `operator!=` 逐通道比較，支援顏色相等性測試（測試 HUD 對比度時使用）。

`namespace nccu::engine::math::Colors` 提供一組 `inline constexpr` 常用顏色常數：`Black / White / RayWhite / DarkGray / Blue / Red / Green / Yellow / Gold / Magenta`，值沿用 raylib 的調色值，方便直接對照。

整個標頭 header-only，不依賴任何外部庫，使上層（IRenderer 介面、UI 繪圖、實體 Render 方法）指定顏色時完全不需引入 raylib 標頭。`RaylibRenderer` 在最終呼叫 raylib 繪製 API 時，才在其內部做 `nccu::engine::math::Color` 到 `::Color` 的欄位對映。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::math::Color`**（struct，aggregate）：`r, g, b, a`（`uint8_t`，預設 `{0,0,0,255}`）。
  - `WithAlpha(uint8_t newA) const noexcept → Color`（`constexpr`）：替換 Alpha 副本。
- **`operator==(Color, Color) noexcept → bool`**（`constexpr`）：逐通道相等。
- **`operator!=(Color, Color) noexcept → bool`**（`constexpr`）：逐通道不等。
- **`nccu::engine::math::Colors` 命名空間**（`inline constexpr` 常數）：
  - `Black {0,0,0,255}`、`White {255,255,255,255}`、`RayWhite {245,245,245,255}`、`DarkGray {80,80,80,255}`、`Blue {0,121,241,255}`、`Red {230,41,55,255}`、`Green {0,228,48,255}`、`Yellow {253,249,0,255}`、`Gold {255,203,0,255}`、`Magenta {255,0,255,255}`。

## 相依與在架構中的位置

- **#include（往外）**：`<cstdint>`（`uint8_t`）——不依賴任何引擎或 raylib 標頭。
- **被誰使用（往內）**：`include/engine/render/IRenderer.h`（繪圖函式參數型別）、`include/engine/render/Renderer.h`、`include/engine/render/TextBuilder.h`、多個實體標頭（`Personas / Player / TransparentUmbrella`）、UI 標頭（`UmbrellaGlyph / HelpPageView`）及大量 `.cpp`（涵蓋幾乎整個 UI 與渲染層）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/math 層的純資料型別，在繪製管線的每個 Draw* 呼叫中作為顏色參數傳遞。

## OO 概念與設計重點

純資料 aggregate struct，`constexpr` 方法確保可在 compile-time 使用。`Colors` 命名空間中的 `inline constexpr` 常數是 C++17 的最佳實踐（避免 ODR 問題的 header-only 常數）。整體設計體現「不依賴具體渲染庫的值型別」原則——渲染層 [arch-dip-renderer](../concepts/arch-dip-renderer.md) 的一部分：`IRenderer` 介面使用此型別，真正呼叫 raylib 的 `RaylibRenderer` 做對映轉換，上層程式碼不需知道 raylib 的顏色格式。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/math/Color.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Color.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP：IRenderer](../concepts/arch-dip-renderer.md)
