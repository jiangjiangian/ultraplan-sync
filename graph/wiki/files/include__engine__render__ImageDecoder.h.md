---
id: "file:include/engine/render/ImageDecoder.h"
type: header
path: include/engine/render/ImageDecoder.h
domain: engine
bucket: render
loc: 44
classes: [DecodedImage]
sources: ["include/engine/render/ImageDecoder.h"]
---
# `ImageDecoder.h`

> **一句定位**：raylib 影像解碼的隔離邊界——把 `::LoadImage` / `::UnloadImage` 等 raylib 呼叫完全封在 `.cpp`，對外只暴露純資料的 `DecodedImage`（RGBA8 像素緩衝），讓遊戲域呼叫者不需 `#include raylib.h`。

## 職責

此標頭定義了一個純資料結構體 `DecodedImage` 和一個自由函式 `LoadRgba8Image`，構成 engine render 層的「影像解碼邊界」。它的設計目標是：凡是需要讀取像素資料（例如 `game/gfx/MaskLoader.h` 讀取碰撞遮罩圖）的模組，都可以消費 `DecodedImage` 的 RGBA8 緩衝，而完全不需引入任何 raylib 符號。

`LoadRgba8Image` 的失敗契約是「不丟例外、回傳空 `DecodedImage`（`Empty() == true`）」，呼叫端以 `Empty()` 判斷再行警告或退路處理。實作細節（`::LoadImage`、`::ImageFormat`、`::UnloadImage`）全藏在 `src/engine/render/ImageDecoder.cpp`，使標頭乾淨、可在不連結 raylib 的測試環境下 `#include`。

命名空間為 `nccu::engine::render`，屬 engine 層 render bucket。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `struct DecodedImage` | 純資料：`width`（int）、`height`（int）、`rgba8`（`std::vector<uint8_t>`，大小 = w×h×4）。 |
| `DecodedImage::Empty()` | 若 `width≤0 \|\| height≤0 \|\| rgba8.empty()` 回傳 `true`；`[[nodiscard]]`、`noexcept`。 |
| `LoadRgba8Image(const std::string& path)` | 自由函式；載入並解碼至 RGBA8 緩衝；失敗回空 `DecodedImage`，絕不丟例外；`[[nodiscard]]`。 |

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<cstdint>`, `<string>`, `<vector>`）；不引入 raylib。
- **被誰使用（往內）**：`include/game/gfx/MaskLoader.h`（讀像素緩衝以建構碰撞遮罩）、`src/engine/render/ImageDecoder.cpp`（實作本標頭宣告的函式）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：啟動期工具；不參與每幀管線。在 MVC 架構中屬 engine 的基礎設施，為 Model 讀取碰撞遮罩服務。

## OO 概念與設計重點

此標頭是**依賴反轉原則（DIP）的邊界點**：遊戲域模組依賴「純 C++ 資料」而非 raylib 型別。`DecodedImage` 是純值型別（純資料，無多型、無繼承），刻意設計得可在無 GPU context 的環境下建構與使用，支援無頭單元測試。

`LoadRgba8Image` 的 no-throw 契約搭配「回傳 sentinel」模式（而非 `std::optional` 或 exception），維持與 C-style raylib API 的風格一致性，並使所有呼叫端都能以簡單 `if (img.Empty())` 處理缺檔情況。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/ImageDecoder.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/ImageDecoder.h) · [← 全檔索引](../files-index.md)
