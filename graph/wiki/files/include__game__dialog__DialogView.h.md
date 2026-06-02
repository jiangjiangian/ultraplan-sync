---
id: "file:include/game/dialog/DialogView.h"
type: header
path: include/game/dialog/DialogView.h
domain: game
bucket: dialog
loc: 27
classes: []
sources: ["include/game/dialog/DialogView.h"]
---
# `DialogView.h`

> **一句定位**：螢幕空間對話框繪製的進入點——自由函式 `DrawDialog` 注入 `IRenderer` 並讀取 `const DialogState&`，對話非作用中時為 no-op。

## 職責

`DialogView.h` 是 game dialog 層的繪製接口，宣告自由函式 `DrawDialog`，屬 MVC 架構的 View 部分（雖掛在 game/dialog 路徑下，但繪製是 View 的職責）。

`DrawDialog(IRenderer& r, const DialogState& d)` 的職責：
- **對話非作用中**（`d.Active()==false`）：不畫任何東西，快速回傳（no-op）。
- **台詞模式**：繪製對話框底板（`Rect{20,320,760,110}` 先填底再細邊框），再把當前行的當前頁各列（`d.CurrentPageRows()`）逐列繪製在 `kBoxTextX`/`kBoxTextY` 起始位置，行高 `kBoxLineH`，字級 `kBoxFontSize`。
- **選單模式**（`d.AtChoice()==true`）：每個選項各佔一列文字，選中者前綴 `"> "`（高亮指示）。
- 所有幾何常數來自 `DialogLayout.h`（由 `DialogState` 的 `#include` 傳遞過來，或 `DialogView.cpp` 直接引入）。

目前為「佔位框」——注解說明待 `resources/assets/ui/` 美術到位後換成 sprite 底板。`IRenderer` 注入使此繪製函式可在測試環境中以 mock renderer 驅動，不需 GL context。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `DrawDialog(IRenderer& r, const DialogState& d)` | 自由函式；注入 renderer 和 const 對話狀態；繪製對話框與文字/選單。 |
| `nccu::engine::render::IRenderer` | 前向宣告；以注入形式（而非直接呼叫 raylib）繪製。 |
| `nccu::DialogState` | 透過 `#include "game/dialog/DialogState.h"` 完整定義。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/dialog/DialogState.h`（`DialogState` 完整定義，含 `DialogChoice`、`kDialogExitLabel`）；`IRenderer` 為前向宣告，不拉入 raylib 相依。
- **被誰使用（往內）**：`src/game/dialog/DialogView.cpp`（`DrawDialog` 實作）、`src/ui/View.cpp`（主遊戲 View 呼叫 `DrawDialog`）；`tests/dialog/test_dialog_box_render.cpp`/`test_dialog_choice_layout.cpp`/`test_dialog_layout.cpp`（測試繪製行為）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：**MVC 的 View**——每幀繪製階段由 `View::Draw()` 呼叫，以 `const DialogState&` 讀取 Model 狀態，透過注入的 `IRenderer` 繪製，完全不修改任何 Model 狀態。

## OO 概念與設計重點

`DrawDialog` 的設計體現了 MVC View 的兩個核心約束：
1. **只讀 Model**（`const DialogState&`）：保證 View 不會意外修改對話狀態。
2. **依賴抽象介面**（`IRenderer&` 而非 raylib）：體現 [DIP/arch-dip-renderer](../concepts/arch-dip-renderer.md)，使測試可注入 mock renderer 驗證繪製邏輯，無需開啟 GL window。

自由函式而非類別方法的設計，讓對話繪製沒有隱式狀態（不依賴物件的成員），任何時候呼叫都是確定性的——唯一的輸入是 `(renderer, state)`，易於在測試中重現和驗證。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogView.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogView.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
