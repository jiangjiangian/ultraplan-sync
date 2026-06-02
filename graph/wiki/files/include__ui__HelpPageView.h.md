---
id: file:include/ui/HelpPageView.h
type: header
path: include/ui/HelpPageView.h
domain: ui
bucket: 
loc: 53
classes: [HelpPageStyle]
sources: ["include/ui/HelpPageView.h"]
---
# `HelpPageView.h`

> **一句定位**：遊戲說明分頁面板的共用渲染 helper——接收 `fillRect` 可呼叫物件，讓遊戲內疊層（IRenderer&）與標題畫面（具體 Renderer）都能重用同一份繪製邏輯而不相互耦合。

## 職責

本標頭解決了「遊戲內暫停說明疊層」與「標題畫面說明頁」幾乎完全相同的繪製程式碼重複問題。兩處原本獨立實作，差異僅在面板顏色、頁碼指示顏色、「返回」標籤文字與其 X 偏移量這幾個參數值。

`DrawHelpPage` 以 `HelpPageStyle` struct 接收這些差異參數，並透過 `std::function<void(Rect, Color)>` 的 `fillRect` 可呼叫物件畫矩形——這讓疊層可以傳入 `IRenderer.DrawRect`，標題畫面可以傳入具體 renderer 的對應方法，而 `DrawHelpPage` 自身不耦合到任何特定渲染類別。文字透過 TextBuilder 直接繪製（與兩處原本的作法相同）。

`DrawHelpPage` 不管背景：疊層的全螢幕遮罩與標題畫面的 Clear 呼叫各不相同，刻意留給呼叫端在「呼叫 `DrawHelpPage` 之前」處理。面板本身（白底矩形）、「遊戲說明」標題、當前說明頁內文（`kGameHelpPages[page]`，行距 17 px、空行 8 px、金色【…】區段標題）、「第 N／M 頁」指示與金邊「返回」標籤由本函式統一繪製。純呈現——不碰 World、不碰輸入，嚴格遵守 MVC。

## 關鍵內容（類別 / 函式 / 資料）

- `HelpPageStyle`（struct）：兩處呼叫端之間不同的參數：
  - `w`、`h`（`float`）：視口寬高。
  - `page`（`int`）：要繪製的頁碼（0 起算，函式內部夾限）。
  - `panelColor`（`Color`）：面板底色（疊層 245α vs 標題 200α）。
  - `indicatorColor`（`Color`）：頁碼指示顏色（疊層亮色 vs 標題暗灰）。
  - `chipLabel`（`std::string_view`）：「返回」標籤文字（疊層「M / E 返回選單」vs 標題「Enter / E 返回」）。
  - `chipLabelXOffset`（`float`）：標籤 X 偏移（疊層 -58 vs 標題 -56）。
- `nccu::ui::DrawHelpPage(fillRect, style)`：繪製面板、標題、當前說明頁內文、頁碼指示與「返回」標籤；接收 `std::function<void(Rect, Color)>` 以抽象矩形繪製，對 renderer 型別保持中立。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Rect.h`、`engine/math/Vec2.h`（版面座標計算）；`engine/math/Color.h`（面板與指示顏色）；`<functional>`（`std::function` 型別）；`<string_view>`。
- **被誰使用（往內）**：`src/app/scenes/TitleScene.cpp`（標題說明頁）；`src/ui/HelpPageView.cpp`（`DrawHelpPage` 的實作）；`src/ui/View.cpp`（傳入視窗尺寸）；`src/ui/overlay/HelpOverlay.cpp`（遊戲內說明疊層）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純 View 層渲染 helper，不在每幀主管線中，由 `HelpOverlay` 的渲染函式於說明開啟時呼叫。

## OO 概念與設計重點

`DrawHelpPage` 透過 `std::function` 注入 `fillRect` 可呼叫物件，是 **Strategy 模式** 的輕量 C++ 實現——把「如何畫矩形」這一行為從「畫什麼」中解耦，讓不同的 renderer 類型都能重用相同的面板繪製邏輯，對應 [pat-strategy](../concepts/pat-strategy.md)。`HelpPageStyle` 作為「差異化參數包」是一種 **Parameter Object** 模式，避免了有七個以上細碎參數的函式簽名。整體設計體現了 **DRY** 原則的嚴格應用：兩處「逐像素一致」但之前各自維護的繪製邏輯，現在統一到一個地方。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/HelpPageView.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/HelpPageView.h) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
