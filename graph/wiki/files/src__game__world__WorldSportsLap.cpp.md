---
id: file:src/game/world/WorldSportsLap.cpp
type: source
path: src/game/world/WorldSportsLap.cpp
domain: game
bucket: world
loc: 59
classes: []
sources: ["src/game/world/WorldSportsLap.cpp"]
---
# `WorldSportsLap.cpp`

> **一句定位**：第三章運動會繞圈邏輯——每幀以 atan2 累計有號角差，滿 92% 圈時設 `kFlagSportsLapDone`，並提供進度取值器供 HUD 環形顯示。

## 職責

此檔屬於 game / world 層，是 `World.cpp` / `WorldSpawn.cpp` 的第三個拆分編譯單元，全部皆為 `World` 的成員。

**`World::UpdateSportsLap() noexcept`**：每幀由章節守衛（`Chapter3_SportsDay` + 旗標未完成）保護，計算玩家座標相對體育場跑道中心（`kSportsTrackCx` / `kSportsTrackCy`）的距離，僅在環帶範圍（90–320 pixel）內才累計角度。首次進入環帶時以 `lapStarted_=true` + `lapPrevAngle_=ang` 定錨；後續每幀以 `atan2(dy, dx)` 取當前角、減去上一幀角（並夾到 ±π 以取最短有號角差），累加到 `lapSwept_`。當 `|lapSwept_| >= 2π × 0.92`（約一整圈，留 8% 寬容）時設 `kFlagSportsLapDone`。此設計使玩家可順時針或逆時針繞圈，且跨越 ±π 邊界（圓的 ±180° 縫隙）時不誤算方向。

**`World::SportsLapProgress() const noexcept`**：若旗標已設則回傳 1.0；否則回傳 `|lapSwept_| / (2π)` 夾在 [0, 1]。HUD 的 `DrawSportsLapRing` 以此繪製點狀進度環。

**`World::SportsLapActive() const noexcept`**：返回是否為 Ch3 且旗標未完成（HUD 是否繪製跑道貼花 / 進度環的守衛）。

## 關鍵內容（類別 / 函式 / 資料）

- `World::UpdateSportsLap() noexcept` — 每幀角度累計；條件：Ch3 且旗標未完成且在環帶範圍；完成條件：`|lapSwept_| >= 2π × 0.92`。
- `World::SportsLapProgress() const noexcept` — 歸一化進度 [0, 1]。
- `World::SportsLapActive() const noexcept` — HUD 顯示條件判斷。
- `lapStarted_` / `lapPrevAngle_` / `lapSwept_` — 純 View / Model 輔助狀態（存於 `World`）；`lapStarted_` 為首次進入環帶的定錨旗標。
- `kSportsTrackCx` / `kSportsTrackCy` — 跑道中心座標常數（來自 `Chapter3Quest.h`）。

## 相依與在架構中的位置

- **#include（往外）**：`World.h`（類別宣告）、`Player.h`（取玩家位置 / 旗標）、`Flags.h`（`kFlagSportsLapDone`）、`Chapter3Quest.h`（`kSportsTrackCx/Cy`）、`Vec2.h`、`<cmath>`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；`UpdateSportsLap` 由 `World::Update` 每幀呼叫；`SportsLapProgress` / `SportsLapActive` 由 `DrawSportsLapRing`（HUD）和 `DrawSportsLapTrack`（地面貼花）讀取。
- **繼承 / 實作 / 體現**：皆為 `World` 的成員。
- **每幀管線 / MVC 角色**：Model 層；`UpdateSportsLap` 在 Movement 之後執行，修改 `Player` 旗標（`kFlagSportsLapDone`）是 Model 寫入。

## OO 概念與設計重點

atan2 + 最短有號角差（`while (d > π) d -= 2π`）是 2D 角度累計的標準技巧，確保繞圈方向兩用且跨越 ±π 縫隙時數值連續。「環帶範圍守衛」（距離 90–320）防止玩家站在圓心或跑出場外時誤算。8% 寬容設計（`0.92` 因子）避免因 60fps 離散取樣在最後一格差一點點而失敗。整個邏輯為 Model 純計算，完全不含渲染呼叫，符合 [MVC](../concepts/arch-mvc.md) 邊界。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/WorldSportsLap.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldSportsLap.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
