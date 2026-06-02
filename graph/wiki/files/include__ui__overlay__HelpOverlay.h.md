---
id: file:include/ui/overlay/HelpOverlay.h
type: header
path: include/ui/overlay/HelpOverlay.h
domain: ui
bucket: overlay
loc: 31
classes: []
sources: ["include/ui/overlay/HelpOverlay.h"]
---
# `HelpOverlay.h`

> **一句定位**：遊戲內「說明」（玩法）疊層的渲染函式聲明——從 `View::RenderOverlays` 抽出，與標題畫面共用 `DrawHelpPage` helper，繪製在暫停選單之上。

## 職責

`DrawHelpOverlay` 是遊戲內暫停說明疊層的渲染函式，從 `View::RenderOverlays` 抽出後獨立成模組。

它是 `World::MenuOpen`、`HelpOpen`、`HelpPage` 三個狀態的純函式：每幀呼叫安全——除非「選單開啟」且「說明開啟」同時成立，否則提前返回（廉價空操作）。繪製時先畫全螢幕遮罩（黑色半透明），再呼叫 `nccu::ui::DrawHelpPage`（`HelpPageView.h` 的共用 helper）繪製說明面板內文與翻頁指示。

關鍵設計：此疊層與標題畫面的說明頁共用同一份 `DrawHelpPage` 實作，確保兩處的說明文字、版面布局與翻頁指示永遠一致，不會漂移。差異（面板顏色、指示顏色、「返回」標籤文字）透過 `HelpPageStyle` 注入。

繪製在暫停選單「之上」（`RenderOverlays` 的呼叫順序確保此函式最後繪製說明層，蓋在選單之上）。純渲染（MVC）：以 `const World&` 唯讀，絕不修改。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawHelpOverlay(r, world, screenW, screenH)`：
  - `r`：`IRenderer&`，繪製介面。
  - `world`：`const World&`，唯讀，讀取 `MenuOpen()`、`HelpOpen()`、`HelpPage()`。
  - 非（`MenuOpen && HelpOpen`）→ 提前返回。
  - 全螢幕遮罩 + `DrawHelpPage(fillRect, style)` 繪製說明面板。
  - `chipLabel`：「M / E 返回選單」。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`World` 與 `IRenderer` 以前向宣告引入）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderOverlays` 最後呼叫，確保疊在選單之上）；`src/ui/overlay/HelpOverlay.cpp`（`DrawHelpOverlay` 的實作，其中引入 `GameHelp.h` 與 `HelpPageView.h`）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 / overlay bucket，在 `RenderOverlays` 階段（螢幕座標，最後渲染層）呼叫。

## OO 概念與設計重點

`DrawHelpOverlay` 透過呼叫 `DrawHelpPage`（`HelpPageView.h`）共享說明頁渲染邏輯，是 **DRY** 的具體應用：兩處說明頁（標題場景與遊戲內）的繪製邏輯只存在一份，避免了說明文字更新時需要在兩個地方同步修改的維護成本。`IRenderer` 注入遵循 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。模組從 `View::RenderOverlays` 中抽出體現了 **SRP**，使說明疊層的邏輯可單獨審閱、測試與修改，不影響其他疊層元件。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/overlay/HelpOverlay.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/HelpOverlay.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
