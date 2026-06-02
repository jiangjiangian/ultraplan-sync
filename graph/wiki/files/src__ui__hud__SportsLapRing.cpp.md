---
id: file:src/ui/hud/SportsLapRing.cpp
type: source
path: src/ui/hud/SportsLapRing.cpp
domain: ui
bucket: hud
loc: 49
classes: []
sources: ["src/ui/hud/SportsLapRing.cpp"]
---
# `SportsLapRing.cpp`

> **一句定位**：HUD 操場校慶繞圈進度環——以 16 個等距圓點（自頂端起順時針）組成的螢幕座標點狀圓環，隨繞圈進度填滿金色。

## 職責

此檔屬於 ui / hud 層，只實作 `DrawSportsLapRing` 一個函式。它以 `world.SportsLapActive()` 守衛（非 Ch3 跑圈中則空操作），讀取 `world.SportsLapProgress()` 為 [0,1] 進度，在螢幕右上角（`screenW - 60, cy=120`）以 16 個 6×6 矩形模擬圓環點。每個點的角度 `a = -2π/4 + frac*2π`（以 -π/2 偏移使起點在圓頂），`frac < prog` 的點繪製亮金色（已完成），否則半透明白色（未完成）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawSportsLapRing(IRenderer&, World&, float W, float /*H*/)` — 唯一公開函式。
- `kDots` = 16 — 環狀點數。
- 圓心：`(screenW - 60, 120)`，半徑 24px；點大小 6×6。
- 金色 `Color{255,230,90,255}` = 已完成；半透明白 `Color{255,255,255,70}` = 未完成。

## 相依與在架構中的位置

- **#include（往外）**：`SportsLapRing.h`、`World.h`（`SportsLapActive` / `SportsLapProgress`）、`IRenderer.h`、`Color.h`/`Rect.h`、`<cmath>`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderHud` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 HUD；與地面的 `DrawSportsLapTrack`（世界座標）互為 HUD 對應物（螢幕座標）。

## OO 概念與設計重點

「點狀環」方案完全以矩形繪製（無圓形原語），成本低、可在 `IRenderer` 抽象下實作，確保 headless 測試友好。進度資料由 `World::SportsLapProgress()` 提供（由 Model 計算），View 只負責視覺映射，符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/hud/SportsLapRing.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/hud/SportsLapRing.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
