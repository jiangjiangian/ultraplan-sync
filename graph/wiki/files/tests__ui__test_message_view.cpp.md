---
id: "file:tests/ui/test_message_view.cpp"
type: test
path: tests/ui/test_message_view.cpp
domain: tests
bucket: ui
loc: 166
classes: [Spy]
sources: ["tests/ui/test_message_view.cpp"]
---
# `test_message_view.cpp`

> **一句定位**：驗證 `DrawHudMessage` 的空訊息/TTL 提前返回、無空白中文字串自動換行、明確換行字元、逐行置中對齊以及不溢出黑框的排版契約。

## 職責

本測試以 `Spy`（IRenderer 假實作）攔截 `DrawHudMessage` 的繪圖呼叫，驗證 HUD 訊息框的所有排版契約。

測試案例涵蓋：
- **空訊息不繪製**：空字串不論存活時間多少，`rects == 0` 且 `texts.empty()`。
- **正常訊息繪製**：短訊息輸出背框（含強調條）與文字列。
- **TTL 截止**：超過 `kHudTtl` 或恰在邊界（包含式）都不繪製。
- **無空白中文換行**：約 40 個中文字在 480px 寬的視窗必須斷成 ≥ 2 列，且拼接回去等於原文（不漏字/不切字）。
- **明確換行字元**：`"\n"` 強制斷成恰好 2 列，各列文字正確分開。
- **逐行置中（DLC 預告）**：`"DLC開發中\n敬請期待"` 兩行，較窄的第二行 `textX > textX[0]`（推右），且兩行的 `textX` 都在框的左緣右側。
- **單行置中**：`textX[0]` 在 `backdrop.x` 到 `backdrop.x + backdrop.width` 之間。
- **不溢出黑框**：長字串換行後每列 `CellWidth(row) * (fontSize*0.5f) ≤ backdrop.width - 36`（內寬）；框本身 ≤ `screenW * 0.72f + 36`。

`Spy` 除記錄 texts/textX/fontSize 外，也追蹤最寬矩形（`backdrop`），使置中/框寬斷言可靠。

## 關鍵內容（類別 / 函式 / 資料）

- `Spy`：實作 `IRenderer`，記錄 `rects`、`sprites`、`texts`、`textX`、`fontSize`；追蹤最寬 `backdrop` 矩形。
- `DrawHudMessage(IRenderer&, text, age, screenW, screenH)` — 被測主要函式。
- `nccu::kHudTtl` — TTL 常數（由 `MessageView.h` 匯出）。
- `nccu::dialog::CellWidth(string)` — 量測東亞字寬的純函式，用於換行斷言。

## 相依與在架構中的位置

- **#include（往外）**：`ui/MessageView.h`（受測主體、`kHudTtl`）、`engine/render/IRenderer.h`（Spy 基底）、`game/dialog/DialogLayout.h`（`CellWidth`）、`engine/math/Rect.h`、`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 實作 `include/engine/render/IRenderer.h`。
- **每幀管線 / MVC 角色**：View 層測試，對應每幀 Draw 路徑中的 HUD 訊息繪製。

## OO 概念與設計重點

以 **Spy** 注入 [IRenderer](../concepts/arch-dip-renderer.md) 驗證逐行置中的唯一可觀察依據（`textX` 位置）。置中測試直接利用「框緊貼最寬行」的幾何性質，讓「較窄行被推右」成為可量化的斷言。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_message_view.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_message_view.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
