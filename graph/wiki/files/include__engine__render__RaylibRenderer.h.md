---
id: "file:include/engine/render/RaylibRenderer.h"
type: header
path: include/engine/render/RaylibRenderer.h
domain: engine
bucket: render
loc: 42
classes: [RaylibRenderer]
sources: ["include/engine/render/RaylibRenderer.h"]
---
# `RaylibRenderer.h`

> **一句定位**：`IRenderer` 介面的唯一具體 raylib 實作——透過委派給 `Renderer` 和 `TextBuilder`，讓所有 `::Draw*` raylib 呼叫只從此類別進入。

## 職責

`RaylibRenderer` 是 `IRenderer` 的 `final` 實作，屬 engine render 層。它把 `IRenderer` 定義的三個繪圖操作（`DrawRect`、`DrawSprite`、`DrawText`）委派給現有的兩個薄包裝器——`Renderer{}` 負責矩形與材質，`TextBuilder` 負責文字——而非直接呼叫 raylib API。

這個類別是「整條 DIP 紅線的末端」：View 層（`ui/View.h`）只知道 `IRenderer*`，`main.cpp` 在組裝時注入具體的 `RaylibRenderer` 實例，視窗 / GL context 之外的測試環境則可注入任何其他 mock 實作。

`RaylibRenderer` 本身無狀態，每次繪製時以預設建構式建立 `Renderer{}` 和 `TextBuilder{}` 即可，構建成本極低。它以 `final` 標記確保不被再繼承，維持 vtable 尺寸與語意的明確性。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class RaylibRenderer final : public IRenderer` | 無狀態的具體實作；override 三個虛函式。 |
| `DrawRect(Rect r, Color c)` | 委派給 `Renderer{}.Rect(r, c)`，以 raylib 填矩形。 |
| `DrawSprite(const Texture& tex, Rect src, Rect dest, Color tint)` | 委派給 `Renderer{}.TextureRect(tex, src, dest, tint)`，支援 sprite sheet 子矩形縮放。 |
| `DrawText(string_view text, Vec2 pos, int size, Color c)` | 委派給 `TextBuilder{…}.At(pos).Size(size).Color(c).Draw()`，走 CJK 字型路徑（若已載入）。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/render/IRenderer.h`（被繼承的抽象介面）、`include/engine/render/Renderer.h`（矩形/材質繪製包裝）、`include/engine/render/TextBuilder.h`（文字繪製建構器）、`include/engine/render/Texture.h`（材質 RAII 控制代碼）。
- **被誰使用（往內）**：`include/ui/View.h`（以 `IRenderer&` 接受注入，此為唯一可組裝的具體型別）、`src/app/main.cpp`（組裝根，建構 `RaylibRenderer` 後傳給 `View`）。
- **繼承 / 實作 / 體現**：繼承 `include/engine/render/IRenderer.h`；體現 DIP（[arch-dip-renderer](../concepts/arch-dip-renderer.md)）。
- **每幀管線 / MVC 角色**：View 層（MVC 的 V）；不參與每幀模擬管線，僅在每幀繪製階段被 `View::Draw()` 調用。

## OO 概念與設計重點

`RaylibRenderer` 是[依賴反轉原則（DIP / arch-dip-renderer）](../concepts/arch-dip-renderer.md)最具體的體現：`ui/` 層的 `View` 以 `IRenderer&` 接收渲染能力，不直接依賴 raylib；`RaylibRenderer` 作為 engine 層的「插頭」在 `main.cpp` 注入。這條設計紅線確保 UI 單元測試可替換為 no-op renderer，無需 GL context 也能驗證繪製邏輯。

由於整個類別都是 inline 定義在標頭中、且每個方法只有一行委派，它是一個典型的**Adapter**（GoF），把 `IRenderer` 介面的呼叫約定轉接到 `Renderer`/`TextBuilder` 的流暢介面。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/RaylibRenderer.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/RaylibRenderer.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
