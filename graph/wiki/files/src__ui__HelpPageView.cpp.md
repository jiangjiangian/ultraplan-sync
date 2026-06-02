---
id: file:src/ui/HelpPageView.cpp
type: source
path: src/ui/HelpPageView.cpp
domain: ui
bucket: 
loc: 73
classes: []
sources: ["src/ui/HelpPageView.cpp"]
---
# `HelpPageView.cpp`

> **一句定位**：遊戲說明分頁的通用渲染函式 `DrawHelpPage`，被遊戲內說明疊層與標題畫面共用，避免兩處各自重寫版面邏輯。

## 職責

此檔屬於 ui 層，只包含一個函式 `DrawHelpPage`，透過注入的 `fillRect` 函式物件（而非直接依賴 `IRenderer`）繪製矩形，使遊戲內說明疊層（`HelpOverlay`）和標題畫面能分別帶入不同的面板透明度與底色，共用相同的版面邏輯。

函式流程：先畫底色面板（`fillRect` + `panelColor`），再畫「遊戲說明」金色標題，接著逐行繪製 `nccu::kGameHelpPages[page]` 中的說明文字（以起首位元組 `0xE3` 判斷是否為 `【…】` 標題列，是則金色、否則白色，行距 17px、空行 8px），最後畫「第 N/M 頁 ←/→ 翻頁」頁碼指示與金框返回按鈕（顯示由呼叫端帶入的 `chipLabel` 字串）。`page` 索引先夾到合法範圍防越界。

此函式使用 `TextBuilder` API 進行渲染，而非直接呼叫 `IRenderer::DrawText`，並完全避免直接取用 `IRenderer`（只用 `fillRect` 閉包）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawHelpPage(const std::function<void(Rect,Color)>& fillRect, const HelpPageStyle& style)` — 唯一公開函式；接受 fillRect 閉包、`HelpPageStyle`（含 `w`、`h`、`page`、`panelColor`、`indicatorColor`、`chipLabel`、`chipLabelXOffset`）。
- `isHeader` lambda — 以 `0xE3` 起首位元組判斷 `【…】` 標題列（UTF-8 U+3010）。
- `HelpPageStyle` — DTO；使兩個呼叫端（疊層 / 標題畫面）的參數差異隔離在結構體中。
- `nccu::kGameHelpPages` / `nccu::kGameHelpPageCount` — 來自 `GameHelp.h` 的靜態說明文字表格。

## 相依與在架構中的位置

- **#include（往外）**：`HelpPageView.h`（`DrawHelpPage` 宣告與 `HelpPageStyle`）、`GameHelp.h`（說明文字）、`TextBuilder.h`（渲染）、`Color.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `HelpOverlay.cpp`（`DrawHelpOverlay` 呼叫）和標題畫面（以不同 `fillRect` / `panelColor` / `chipLabel` 呼叫）共用。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；在 `RenderOverlays → DrawHelpOverlay` 中呼叫，僅在 `HelpOpen` 時才執行（其他時間空操作）。

## OO 概念與設計重點

`fillRect` 參數是策略注入（[Strategy](../concepts/pat-strategy.md) / DI），讓同一個版面邏輯可以因為「矩形 alpha 不同」這個唯一差異而被兩個不同的呼叫端使用，避免了代碼重複。`HelpPageStyle` DTO 封裝呼叫端之間的參數差異，符合 OCP：增加新呼叫端只需提供不同的 `HelpPageStyle`，不修改 `DrawHelpPage` 本身。以起首位元組 `0xE3` 判斷中文標題列是特化的 UTF-8 處理技巧，節省完整解碼的成本，且在此場景下完全安全（`kGameHelpPages` 的內容受嚴格管控）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/HelpPageView.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/HelpPageView.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
