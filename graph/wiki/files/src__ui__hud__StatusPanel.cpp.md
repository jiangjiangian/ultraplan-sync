---
id: file:src/ui/hud/StatusPanel.cpp
type: source
path: src/ui/hud/StatusPanel.cpp
domain: ui
bucket: hud
loc: 158
classes: []
sources: ["src/ui/hud/StatusPanel.cpp"]
---
# `StatusPanel.cpp`

> **一句定位**：左上角 HUD 狀態面板——操作提示（WASD / Tab / M）、業力 / 雨傘、金錢、所在建築（條件性）、章節名稱、降雨計量六至七行，以動態計算寬度的半透明底板呈現。

## 職責

此檔屬於 ui / hud 層，實作 `DrawStatusPanel` 一個函式。它從 `World` / `Player` 讀取所有即時數值，組裝各行字串，估計面板寬度，繪製深色半透明底板，再以 `TextBuilder` 逐行繪製。

行組成：`"WASD: move    E: pick up"` + `"Tab: 物品欄   M: 選單"`（均為 ASCII + CJK 混合）、`"karma: N   umbrella: yes/no"`、`"金幣: N 元"`（金色）、選用的 `"Inside: <建築名>"`（金色）、章節名稱（金色）、`"<等級字形> rain: N%"`（白 / 金 / 紅隨雨量漸變）。

面板寬度動態計算：以 UTF-8 起首位元組數 + CJK 字數加倍近似各行顯示寬度，取各行最大值後乘 `kHudSize * 0.55f`（每字形 0.55 個字體大小的像素）+ padding。降雨讀數前置等級字形（`RainTierPrefix`）讓三個壓力等級在色覺無障礙情境下也能辨識（不僅依賴顏色）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawStatusPanel(IRenderer&, World&)` — 唯一公開函式，156 行。
- `glyphsOf` lambda — 計算字串中 UTF-8 碼點數（起首位元組計數）。
- `cjkGlyphsOf` lambda — 計算 3-byte CJK 碼點數（起首位元組 `0xE0..`）用於全形寬度修正。
- `kHudX`=10、`kHudY`=10、`kLineH`=20、`kHudSize`=16、`kPad`=6 — 版面常數。
- `RainTierPrefix(rm)` — 來自 `RainHud.h`，前置等級字形以支援色覺無障礙。
- 顏色：白（WASD）、柔灰（Tab/M 次要）、白（業力）、金（金幣 / 建築 / 章節）、白/金/紅（降雨）。

## 相依與在架構中的位置

- **#include（往外）**：`StatusPanel.h`、`World.h`、`Player.h`、`RainHud.h`（`RainTierPrefix`）、`IRenderer.h`、`TextBuilder.h`、`Color.h`/`Rect.h`/`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderHud` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 HUD；純讀 `const World&`，不修改 Model。

## OO 概念與設計重點

CJK 字寬估計（起首位元組計數 + 全形字數加倍）在不使用字型量測 API 的情況下給出合理的面板寬度，與全專案的「字格模型」一致（不另建私有估計，避免走樣）。`RainTierPrefix` 的加入體現了無障礙設計（WCAG 的「非僅靠顏色傳遞資訊」原則），是一個小但重要的 UX 改善。純讀 View，符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/hud/StatusPanel.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/StatusPanel.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
