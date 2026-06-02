---
id: file:src/game/dialog/DialogView.cpp
type: source
path: src/game/dialog/DialogView.cpp
domain: game
bucket: dialog
loc: 61
classes: []
sources: ["src/game/dialog/DialogView.cpp"]
---
# `DialogView.cpp`

> **一句定位**：對話框的渲染函式——依 `DialogState` 的當前狀態，在螢幕底部繪製台詞文字或選項選單，並加上 `▼` 更多提示。

## 職責

`DialogView.cpp` 實作自由函式 `DrawDialog(IRenderer& r, const DialogState& d)`，是整個對話系統唯一的渲染入口，屬 View 層。它只讀取 `const DialogState&`，完全不修改遊戲狀態。

**對話方塊底板**：先以 `DrawRect` 繪製白色底板（`kBoxX/kBoxY/kBoxW/kBoxH`），再覆蓋一條深灰色標題橫條（高 2.0f），使框體邊框可見。

**台詞模式**：`!d.AtChoice()` 時，從 `d.CurrentPageRows()` 取已斷行的當前頁列，逐行以 `DrawText` 繪製（行距 `kBoxLineH`），並在 `d.HasMore()` 時於框體右下角繪製 `▼`（U+25BC，UTF-8 `\xE2\x96\xBC`）作為「還有更多」提示。

**選項模式**：`d.AtChoice()` 時，對每個選項標籤呼叫 `WrapToCells("> " + label, kBoxCells)` 斷行，以 `"> "` 前綴標示被選中選項（`i == d.ChoiceCursor()`），未選中用 `"  "` 保持縮排對齊。每個選項的列組先堆疊再加 `kInterChoiceGap`（8.0f px）的選項間距，使三個選項在 110px 面板內清晰可讀（3 列×22 + 2 間距×8 = 82px < kBoxH）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawDialog(IRenderer& r, const DialogState& d)`：唯一公開入口。
- `constexpr float kInterChoiceGap = 8.0f`：選項間距（局部常數）。
- `kBoxX / kBoxY / kBoxW / kBoxH / kBoxTextX / kBoxTextY / kBoxFontSize / kBoxLineH / kBoxCells / kBoxRowsPerPage`：對話框排版常數，引自 `DialogLayout.h`。
- `Colors::RayWhite / Colors::DarkGray / Colors::Black`：渲染顏色，引自 `Color.h`。
- `WrapToCells(string, int)`：選項標籤斷行，引自 `DialogLayout.h`。
- `d.AtChoice()` / `d.Choices()` / `d.ChoiceCursor()` / `d.CurrentPageRows()` / `d.HasMore()`：讀取 `DialogState`。

## 相依與在架構中的位置

- **#include（往外）**：`DialogView.h`、`DialogLayout.h`（排版常數與 `WrapToCells`）、`IRenderer.h`（`DrawRect` / `DrawText`）、`Rect.h` / `Vec2.h` / `Color.h`（幾何與顏色型別）。
- **被誰使用（往內）**：—（葉節點；由 View 根（`RaylibRenderer` 驅動的渲染管線）直接呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：View 層，屬 [MVC](../concepts/arch-mvc.md) 的渲染側。只讀 `const DialogState&`，所有 `Draw*` 都通過 `IRenderer` 介面，符合 [DIP 渲染器](../concepts/arch-dip-renderer.md)。

## OO 概念與設計重點

本檔嚴格遵守 [DIP 渲染器](../concepts/arch-dip-renderer.md)：所有繪製呼叫均透過 `IRenderer`，不直接使用 raylib。View 只讀 Model（`const DialogState&`），體現 [MVC](../concepts/arch-mvc.md) 的單向依賴。選項標籤以 `WrapToCells` 斷行再逐行繪製，而非假設標籤單行，是防止長選項標籤溢出框體的防禦性設計，在第四章三選項終局場景（「體諒助教的辛勞」等長標籤）尤其重要。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogView.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogView.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP 渲染器](../concepts/arch-dip-renderer.md)
