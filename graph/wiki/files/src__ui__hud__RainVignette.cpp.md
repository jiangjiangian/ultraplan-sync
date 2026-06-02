---
id: file:src/ui/hud/RainVignette.cpp
type: source
path: src/ui/hud/RainVignette.cpp
domain: ui
bucket: hud
loc: 44
classes: []
sources: ["src/ui/hud/RainVignette.cpp"]
---
# `RainVignette.cpp`

> **一句定位**：降雨壓力暈影——以四條邊框帶在螢幕邊緣加深，在雨量 ≥60（微弱）或 ≥85（較強）時提供視覺緊迫感警示。

## 職責

此檔屬於 ui / hud 層，只實作 `DrawRainVignette` 一個函式。它讀取 `player->GetRainMeter()` 並以兩段門檻決定邊框 alpha（`≥85 → 90`、`≥60 → 45`、`<60 → 0 = 空操作`），再以四條矩形邊框帶（上下左右，厚度 = `min(W,H) * 0.12f`）繪製黑色暈影。此方案無需全螢幕紋理或額外記憶體，且渲染結果具決定性（不依賴任何隨機或時間狀態）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawRainVignette(IRenderer&, World&, float W, float H)` — 唯一公開函式。
- 門檻：`rm >= 85` → alpha=90（較強暗角）；`rm >= 60` → alpha=45（微弱暗角）；`rm < 60` → 空操作。
- 邊框厚度 = `min(W, H) * 0.12f`，依視窗尺寸自適應。

## 相依與在架構中的位置

- **#include（往外）**：`RainVignette.h`、`Player.h`、`World.h`、`IRenderer.h`、`Color.h`/`Rect.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderHud` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 HUD；純讀 `GetRainMeter()`，不修改 Model。

## OO 概念與設計重點

極簡設計：44 行、無類別、無狀態，只由雨量當幀值推導視覺效果，是純渲染函式的範例。四矩形邊框方案比全螢幕渲染更省成本，且結果具決定性（符合 harness bit-for-bit 不變的設計要求）。符合 [MVC](../concepts/arch-mvc.md) 的 View 零副作用原則。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/hud/RainVignette.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/RainVignette.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
