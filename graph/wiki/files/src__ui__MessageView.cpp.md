---
id: file:src/ui/MessageView.cpp
type: source
path: src/ui/MessageView.cpp
domain: ui
bucket: 
loc: 112
classes: []
sources: ["src/ui/MessageView.cpp"]
---
# `MessageView.cpp`

> **一句定位**：`DrawHudMessage` 的實作——在螢幕底部（或底部上方）繪製淡入淡出的提示橫幅，以 EAW 感知換行和雙插槽（Top / Bottom）支援兩行並存訊息。

## 職責

此檔屬於 ui 層，實作單一公開函式 `DrawHudMessage`，負責渲染 `World::HudMessage` / `HudAge` 的短暫 `ShowMessage` 提示橫幅。

核心設計要點：
- **淡出計算**：`remaining = kHudTtl - age`；`HudToastFadeT(remaining, kHudFade, reducedMotion)` 計算 [0,1] 透明度；`reducedMotion=true` 時塌縮為硬切（維持不透明直到 TTL 邊界）。
- **EAW 換行**：以 `WrapToBox(message, maxWidth)` 呼叫 `nccu::dialog::WrapToCells`，用字格預算（`maxWidth / kPxPerCell`）換行，支援 `'\n'` 強制斷行，確保過長訊息不溢出面板。
- **置中排版**：方框寬取最寬換行行，水平置中；每行以 `(innerW - lineW) * 0.5f` 在方框內再次置中。
- **雙插槽**：`HudSlot::Bottom` 錨在螢幕底部 `kMarginB` 上方；`HudSlot::Top` 以固定的單行底部高度 + `kSlotGap` 浮在 Bottom 上方（不讀當前 Bottom 高度，避免兩插槽互相耦合）。
- **背板 + 頂線**：方框背板 `Color{20,20,20,a}` + 頂部 2px 白線，與提示一起淡出。

字格模型常數 `kPxPerCell = kFontSize * 0.5f`（每 EAW 格約 sz/2 px）與 `EndingView` / `ChapterCard` / `DialogLayout` 完全共用，確保單一事實來源。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawHudMessage(IRenderer&, string&, float age, float W, float H, bool reducedMotion, HudSlot)` — 唯一公開函式。
- `kFontSize` = 18、`kPadX` = 18、`kPadY` = 12、`kLineH` = 24、`kMarginB` = 28 — 版面常數。
- `kPxPerCell` = `kFontSize * 0.5f` = 9 — EAW 字格像素寬，與全域文字量測模型一致。
- `TextWidthPx(string&)` — EAW 像素寬估計。
- `WrapToBox(string&, float maxWidth)` — 呼叫 `WrapToCells`，以字格預算換行。
- `HudSlot::Top` / `HudSlot::Bottom` — 兩個垂直插槽；`kSlotGap` = 12px 固定間距。

## 相依與在架構中的位置

- **#include（往外）**：`MessageView.h`（`DrawHudMessage` 宣告）、`ReducedMotion.h`（`HudToastFadeT`）、`DialogLayout.h`（`WrapToCells` / `CellWidth`）、`IRenderer.h`、`Rect.h`/`Vec2.h`/`Color.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderOverlays` 兩次呼叫（Top 插槽 + Bottom 插槽）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層疊層；在 `RenderOverlays` 中對話框之上、章節字卡之下繪製。

## OO 概念與設計重點

雙插槽設計以固定間距讓兩條訊息（章節通關提示 Top + 日常 ShowMessage Bottom）並存，同時避免兩個插槽互相量測高度（避免耦合）。`WrapToBox` 統一使用 `WrapToCells` 的字格預算確保 EAW 換行正確，且 `'\n'` 強制斷行行為與 `DialogView` 完全一致——「單一換行語意」的設計統一。`reducedMotion` 的硬切處理直接整合進 `HudToastFadeT` 輔助函式，保持 `DrawHudMessage` 的主邏輯乾淨。[DIP Renderer](../concepts/arch-dip-renderer.md) 讓渲染呼叫全部走 `IRenderer`，可做 headless 測試。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/MessageView.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/MessageView.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
