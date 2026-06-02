---
id: "file:tests/dialog/test_dialog_box_render.cpp"
type: test
path: tests/dialog/test_dialog_box_render.cpp
domain: tests
bucket: dialog
loc: 64
classes: [Spy]
sources: ["tests/dialog/test_dialog_box_render.cpp"]
---
# `test_dialog_box_render.cpp`

> **一句定位**：以 `Spy` IRenderer 在無 GL 環境下驗證 `DrawDialog` 的對話框算繪：未啟用時無輸出、啟用時繪製面板與台詞、選項模式下每個選項一段文字且被選中者前綴 `"> "`。

## 職責

本檔為 `DrawDialog(IRenderer&, DialogState&)` 提供算繪輸出的單元測試。由於測試環境無 GL，使用 `Spy` 攔截所有 `DrawText/DrawRect/DrawSprite` 呼叫，不需真正的顯示卡。

**`Spy` struct**：繼承 `IRenderer`，記錄 `rects`（矩形繪製次數）、`sprites`（精靈次數）與 `texts`（文字字串向量），讓 case 能驗證實際發出的台詞與選項內容。

三個 `TEST_CASE`：
1. **未啟用不畫任何東西**：`DialogState d`（預設未開啟）呼叫 `DrawDialog`，`s.rects == 0` 且 `s.texts.empty()`。
2. **啟用單行畫出面板與台詞**：`d.Open({"hello"})` 後，`s.rects >= 1`（面板底色）且 `s.texts[0] == "hello"`，驗證正常台詞模式。
3. **選項模式每個選項各一段文字，被選中者前綴 `"> "`**：`d.Open({"intro"}, {{"refuse",0},{"accept",-5}})` 再 `d.Advance()` 進入選項模式；`s.texts.size() == 2`；`s.texts[0]` 以 `"> "` 開頭（選項 0 被選中）；`s.texts[1]` 以 `"  "` 開頭（未選中）。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Spy : IRenderer`：`rects`、`sprites`、`texts`，無 GL 的算繪攔截器。
- `DrawDialog(Spy, DialogState)`（被測函式）：負責依 `DialogState` 目前狀態決定繪製內容。
- `TEST_CASE("未啟用的對話不畫任何東西")`
- `TEST_CASE("啟用的單行對話畫出面板與當前台詞文字")`
- `TEST_CASE("選項模式每個選項各畫一段文字，被選中者前綴 '> '")`

## 相依與在架構中的位置
- **#include（往外）**：`include/engine/render/IRenderer.h`、`include/game/dialog/DialogState.h`、`include/game/dialog/DialogView.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 繼承 `IRenderer`（`include/engine/render/IRenderer.h`）
- **每幀管線 / MVC 角色**：驗證 View 層（`DrawDialog`）的輸出合約，對應 MVC 的 View 部分

## OO 概念與設計重點

`Spy` 體現了 [arch-dip-renderer](../concepts/arch-dip-renderer.md) 的測試端：`IRenderer` 介面將 View 邏輯與 GL 解耦，任何實作（含測試間諜）都能插入。這也是「測試替身（Test Spy）」模式的標準應用——記錄呼叫而非真正執行，讓 case 能對「怎麼畫」下斷言，而非「畫出什麼像素」。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_box_render.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_box_render.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-dip-renderer](../concepts/arch-dip-renderer.md)
