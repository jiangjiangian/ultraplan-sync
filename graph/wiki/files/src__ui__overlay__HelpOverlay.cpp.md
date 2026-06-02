---
id: file:src/ui/overlay/HelpOverlay.cpp
type: source
path: src/ui/overlay/HelpOverlay.cpp
domain: ui
bucket: overlay
loc: 48
classes: []
sources: ["src/ui/overlay/HelpOverlay.cpp"]
---
# `HelpOverlay.cpp`

> **一句定位**：遊戲內說明疊層的渲染——MenuOpen 且 HelpOpen 時，以全螢幕遮罩 + 共用 `DrawHelpPage` 呈現當前說明頁，與標題畫面共用相同的說明排版邏輯。

## 職責

此檔屬於 ui / overlay 層，只實作 `DrawHelpOverlay` 一個函式。它以 `world.MenuOpen() && world.HelpOpen()` 守衛（否則直接返回，空操作），畫一個全螢幕黑色遮罩（alpha 205，疊在暫停選單之上），再以注入的 `fillRect` 閉包呼叫共用的 `nccu::ui::DrawHelpPage`，傳入：面板色 `{18,20,28,245}`、指示色 `{200,200,210,255}`、chipLabel `"M / E 返回選單"`（X 偏移 -58）。頁碼由 `world.HelpPage()` 提供，並夾到合法範圍。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawHelpOverlay(IRenderer&, World&, float W, float H)` — 唯一公開函式。
- `fillRect` lambda — `[&r](Rect, Color) { r.DrawRect(...); }`，注入給 `DrawHelpPage`。
- `HelpPageStyle` 參數：`W`、`H`、`page`（來自 `world.HelpPage()`）、panel/indicator color、chipLabel `"M / E 返回選單"`。

## 相依與在架構中的位置

- **#include（往外）**：`HelpOverlay.h`、`World.h`（`MenuOpen`/`HelpOpen`/`HelpPage`）、`IRenderer.h`、`Color.h`/`Rect.h`、`GameHelp.h`（`kGameHelpPageCount`）、`HelpPageView.h`（`DrawHelpPage`/`HelpPageStyle`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderOverlays` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層疊層；在暫停選單之上繪製，`MenuOpen && HelpOpen` 時才顯示。

## OO 概念與設計重點

此檔的核心價值是「去重」：透過 `fillRect` 注入（[Strategy](../concepts/pat-strategy.md) / DI），讓遊戲內疊層和標題畫面共用 `DrawHelpPage` 的排版邏輯，而各自帶入不同的底色、指示色和返回按鈕標籤。`"M / E 返回選單"` 與標題畫面的 `"Enter 繼續"` 等按鈕文字，是兩個呼叫端之間僅有的行為差異。純讀 Model，符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/overlay/HelpOverlay.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/HelpOverlay.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [MVC](../concepts/arch-mvc.md)
