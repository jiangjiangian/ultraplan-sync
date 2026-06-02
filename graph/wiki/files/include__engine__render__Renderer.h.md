---
id: "file:include/engine/render/Renderer.h"
type: header
path: include/engine/render/Renderer.h
domain: engine
bucket: render
loc: 117
classes: [Renderer, nccu, nccu, nccu]
sources: ["include/engine/render/Renderer.h"]
---
# `Renderer.h`

> **一句定位**：raylib 基本繪圖呼叫的薄包裝——以引擎的 `Color`/`Rect`/`Vec2` 型別對外，把 `::ClearBackground`/`::DrawRectangleRec`/`::DrawTexturePro` 等 raylib 符號收斂在此一處。

## 職責

`Renderer` 類別是 engine render 層的「即時繪圖瑞士刀」。它以流暢介面（每個方法回傳 `*this`）包裝六個最常用的 raylib 繪圖原語：清背景、填矩形、矩形外框、單像素點、整張材質、材質子矩形（`TextureRect`，用於 sprite sheet 切幀與縮放）。

設計重點：類別本身是 header-only（所有方法 inline 在 `.h`），無狀態，可在每幀的任何時間點以 `Renderer{}` 建立並立即呼叫，不需持有長壽實例。標頭直接 `#include "raylib.h"`，是 engine render 層中「明確允許帶入 raylib」的薄層之一（相對於 `ImageDecoder.h` 的「把 raylib 完全封在 .cpp」）。

`TextureRect` 的 `dest` 與 `src` 尺寸可不同，raylib 的 `::DrawTexturePro` 會做縮放——這支援了選角畫面中角色預覽格的放大展示，也支援了 sprite sheet 動畫幀的精確抽取。

方法名 `Rect` 和 `Texture` 在類別作用域內遮蔽同名型別，故參數型別以詳述形式（`struct nccu::engine::math::Rect`、`class Texture`）標注——這是一個刻意記錄在注解中的語言細節。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class Renderer` | 無狀態的 header-only 即時繪圖包裝；所有方法 `noexcept` 並回傳 `*this`。 |
| `Clear(Color c)` | 呼叫 `::ClearBackground`，轉換引擎 `Color` 到 raylib `::Color`。 |
| `Rect(Rect r, Color c)` | 呼叫 `::DrawRectangleRec` 繪製填滿矩形。 |
| `RectLines(Rect r, Color c, float thickness)` | 呼叫 `::DrawRectangleLinesEx` 繪製外框，thickness 預設 1.0f。 |
| `Pixel(Vec2 p, Color c)` | 呼叫 `::DrawPixelV` 繪製單像素點。 |
| `Texture(const Texture& tex, Vec2 pos, Color tint)` | 呼叫 `::DrawTextureV`，整張材質原尺寸繪製於 `pos`。 |
| `TextureRect(const Texture& tex, Rect src, Rect dest, Color tint)` | 呼叫 `::DrawTexturePro`，支援子矩形切片與縮放；tint 預設 `Colors::White`。 |

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（直接使用 raylib API）、`include/engine/math/Color.h`、`include/engine/math/Rect.h`、`include/engine/render/Texture.h`、`include/engine/math/Vec2.h`。
- **被誰使用（往內）**：`include/engine/render/RaylibRenderer.h`（`DrawRect`/`DrawSprite` 委派給此）、`src/app/scenes/CharacterSelectScene.cpp`、`src/app/scenes/LoadingScene.cpp`、`src/app/scenes/TitleScene.cpp`、`src/ui/View.cpp`（直接用 `Renderer{}` 繪製世界層）。
- **繼承 / 實作 / 體現**：—（無繼承關係；是 `RaylibRenderer` 委派的實作工具，而非 `IRenderer` 的直接實作）
- **每幀管線 / MVC 角色**：View 層（MVC 的 V）的繪圖原語；在每幀繪製階段被各 Scene / View 呼叫，不參與模擬管線。

## OO 概念與設計重點

`Renderer` 採**流暢介面（Fluent Interface）**設計，使同一幀內的多個繪圖呼叫可串接為一條語句，提升可讀性。它是標準的**Facade**模式的一環：把六個不同簽名的 raylib 繪圖呼叫統一成一個以引擎型別為語言的 API，隔離 raylib 命名慣例的細節（如 `::Rectangle`、`::Vector2` 等 C 結構體）。

Header-only 設計保持了零額外 `.cpp` 的低維護成本；由於類別無狀態，inline 也無 ODR 問題。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/engine/render/Renderer.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Renderer.h) · [← 全檔索引](../files-index.md)
