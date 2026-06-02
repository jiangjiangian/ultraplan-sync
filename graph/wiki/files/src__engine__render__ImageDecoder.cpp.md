---
id: "file:src/engine/render/ImageDecoder.cpp"
type: source
path: src/engine/render/ImageDecoder.cpp
domain: engine
bucket: render
loc: 40
classes: []
sources: ["src/engine/render/ImageDecoder.cpp"]
---
# `ImageDecoder.cpp`

> **一句定位**：以 raylib 載入圖片並統一轉為 RGBA8 格式的解碼函式實作，上層不需處理各種來源像素格式。

## 職責

`LoadRgba8Image(const std::string& path)` 是此檔案唯一的函式，執行三個步驟：

1. 呼叫 raylib 的 `::LoadImage(path.c_str())`；若失敗（`data == nullptr` 或 `width <= 0`）則先釋放可能已配置的緩衝區再回傳空的 `DecodedImage{}`。
2. 呼叫 `::ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)` 統一像素格式，無論原始圖片是 RGB、灰階或帶 alpha 的其他格式，均轉為 RGBA8。
3. 將像素資料以 `vector::assign(src, src + pixelCount)` 複製進 `out.rgba8`，再呼叫 `::UnloadImage(img)` 釋放 raylib 緩衝區，避免持有兩份資料。

回傳的 `DecodedImage` 含 `width`、`height`（int）與 `rgba8`（`vector<uint8_t>`），使上層的 Texture / 碰撞遮罩計算等完全與 raylib 格式細節解耦。

## 關鍵內容（類別 / 函式 / 資料）

- `LoadRgba8Image(const std::string& path) -> DecodedImage` — 載入 → 格式統一 → 複製 → 釋放；失敗回傳空 `DecodedImage{}`。
- `DecodedImage` — 回傳結構（定義於標頭）：`int width`、`int height`、`vector<uint8_t> rgba8`。

## 相依與在架構中的位置
- **#include（往外）**：`ImageDecoder.h`；raylib（`::LoadImage`、`::ImageFormat`、`::UnloadImage`、`PIXELFORMAT_UNCOMPRESSED_R8G8B8A8`）
- **被誰使用（往內）**：—（由 Texture 快取、碰撞遮罩等需要 CPU 側像素資料的模組使用）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / render 層；資源載入期（非每幀）

## OO 概念與設計重點

此函式是單職責（SRP）的純工具函式：只做「載入 + 格式統一 + 複製 + 釋放」，不涉及 GPU 上傳（由 Texture 負責）或業務邏輯。「先釋放再回傳空值」的失敗路徑防止 raylib 內部緩衝區洩漏，是明確的 RAII 邊界條件處理。`pixelCount` 以 `static_cast<size_t>` 計算避免 32 位元溢位（`width × height × 4u`）。格式統一在 CPU 端執行，使呼叫端可對 RGBA8 做假設，降低圖形格式多樣性帶來的複雜度。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/render/ImageDecoder.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/render/ImageDecoder.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
