---
id: file:include/engine/render/IRenderer.h
type: header
path: include/engine/render/IRenderer.h
domain: engine
bucket: render
loc: 56
classes: [IRenderer]
sources: ["include/engine/render/IRenderer.h"]
---
# `IRenderer.h`

> **一句定位**：抽象繪圖服務介面，把所有 raylib 繪圖呼叫隔離在其後，是 [arch-dip-renderer](../concepts/arch-dip-renderer.md) 的核心——讓 Model 層不直接依賴 raylib，同時使測試可在無 GL context 下驗證繪圖呼叫。

## 職責

`IRenderer` 是 engine/render 層最重要的抽象邊界。它定義三個純虛擬繪圖方法，每個方法都用引擎型別（`Rect / Vec2 / Color / Texture / string_view`）作為參數，完全不出現任何 raylib 型別。

三個繪圖方法：
- `DrawRect(Rect r, Color c)`：填滿矩形，用於 HUD 背景、對話框、遮罩等。
- `DrawSprite(const Texture& tex, Rect src, Rect dest, Color tint)`：材質子矩形縮放貼到目標矩形，是所有 sprite（玩家、NPC、道具、地圖元素）繪製的統一入口；`tint` 預設白色（不改色）。
- `DrawText(string_view text, Vec2 pos, int size, Color c)`：繪製文字；`RaylibRenderer` 的實作依 CJK 字型是否已載入選擇 `DrawTextEx`（CJK 字型）或 `DrawText`（ASCII 退路）。

設計意圖：`IDrawable::Render(IRenderer& renderer)` 只接受此介面，因此所有實體（傘家族、NPC、道具、HUD 元素）的繪製程式碼對 raylib 完全無知。測試可以用 spy / mock 實作 `IRenderer` 在無 GL context 的情境下驗證繪圖呼叫（是否呼叫了正確的繪製方法、正確的座標 / 顏色等）。

`Texture` 只以前向宣告引入，防止 raylib 的 `Texture2D` 型別滲入此標頭的包含樹。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::render::IRenderer`**（抽象介面）：
  - `DrawRect(Rect r, Color c) = 0`：填滿矩形。
  - `DrawSprite(const Texture&, Rect src, Rect dest, Color tint = Colors::White) = 0`：材質 sprite 貼圖。
  - `DrawText(string_view text, Vec2 pos, int size, Color c) = 0`：文字繪製。
  - 虛擬解構子。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Color.h`、`include/engine/math/Rect.h`、`include/engine/math/Vec2.h`（繪圖方法的參數型別）；`<string_view>`；以前向宣告引入 `Texture`。
- **被誰使用（往內）**：`include/engine/render/RaylibRenderer.h`（具體實作）；眾多實體 / UI `.cpp`（傘家族、NPC、HUD 元素等）直接依賴此介面繪製；多個測試以 mock 繼承此介面驗證渲染行為。
- **繼承 / 實作 / 體現**：被 `RaylibRenderer` 實作；多個測試用 spy / mock 繼承（`test_dialog_box_render / test_umbrella_render / test_message_view` 等）。realizes [DIP：IRenderer](../concepts/arch-dip-renderer.md)。
- **每幀管線 / MVC 角色**：engine/render 層的 View / Controller 邊界；每幀 `Draw / Render` 路徑的統一抽象入口。在 MVC 架構中：View 層透過此介面輸出，Model 的 `IDrawable::Render` 接受此介面——既讓 View 控制渲染策略，又讓 Model 具備可測試的繪製能力。

## OO 概念與設計重點

`IRenderer` 是 [DIP（依賴倒置原則）](../concepts/arch-dip-renderer.md) 最直接的應用：高層（Model 的 `IDrawable`、UI 元素）不直接依賴低層（raylib），而是依賴此抽象介面；低層（`RaylibRenderer`）也依賴此抽象。依賴方向：Model → IRenderer ← RaylibRenderer，中間有抽象隔離。

前向宣告 `Texture`（而非 `#include`）防止 raylib 的 `Texture2D` 透過 `Texture.h` 滲入此標頭，維持「raylib 不進 Model / IRenderer」的架構紅線。測試用 spy 繼承 `IRenderer` 記錄呼叫，使繪圖邏輯可在 CI 下驗證，是本系統最有價值的測試分層之一。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/IRenderer.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/IRenderer.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP：IRenderer](../concepts/arch-dip-renderer.md)
