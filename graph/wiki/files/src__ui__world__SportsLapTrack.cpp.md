---
id: file:src/ui/world/SportsLapTrack.cpp
type: source
path: src/ui/world/SportsLapTrack.cpp
domain: ui
bucket: world
loc: 66
classes: []
sources: ["src/ui/world/SportsLapTrack.cpp"]
---
# `SportsLapTrack.cpp`

> **一句定位**：操場校慶繞圈的地面跑道貼花——以 48 個點描出體育場外形（直線 + 半圓），已通過的點消除，使跑道隨玩家完成進度縮短。

## 職責

此檔屬於 ui / world 層，只實作 `DrawSportsLapTrack` 一個函式。以 `world.SportsLapActive()` 守衛（非 Ch3 跑圈中則空操作），讀取 `world.SportsLapProgress()` 進度，以 48 個白色 10×10 矩形在世界座標描出體育場外形（上方直線 → 右端東側半圓 → 下方直線 → 左端西側半圓），依 `kSportsTrackHalfLen`（直線半長）和 `kSportsTrackR`（半圓半徑）的周長參數計算各點的世界座標。

已走過的點（`frac < prog`）跳過（消除），實現「跑道隨進度縮短」的視覺回饋。必須在底圖之後、建築 / 物件繪製之前呼叫（地面貼花語意），使建築和跑者疊在線條之上。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawSportsLapTrack(IRenderer&, World&)` — 唯一公開函式。
- `kDots`=48 — 總點數（比 SportsLapRing 的 16 點細密，適合地面標線）。
- 體育場幾何：上方直線（cx-a 至 cx+a，y=cy-rad）→ 右半圓（θ: 0→π）→ 下方直線（cx+a 至 cx-a，y=cy+rad）→ 左半圓（θ: 0→π）；`kSportsTrackHalfLen`、`kSportsTrackR` 來自 `Chapter3Quest.h`。
- 點顏色：`Color{255,255,255,240}`（跑道標線白）。
- 「已走過點消除」：`if (frac < prog) continue;`。

## 相依與在架構中的位置

- **#include（往外）**：`SportsLapTrack.h`、`Chapter3Quest.h`（`kSportsTrackCx/Cy`、`kSportsTrackHalfLen`、`kSportsTrackR`）、`World.h`（`SportsLapActive`/`SportsLapProgress`）、`IRenderer.h`、`Color.h`/`Rect.h`、`<cmath>`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderWorld` 在 `CameraScope` 內、底圖繪製之後、畫家排序之前呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（world 子層）；地面貼花，世界座標，每幀 `RenderWorld` 呼叫。是 HUD 的 `DrawSportsLapRing`（螢幕座標）的地面對應物。

## OO 概念與設計重點

體育場外形的參數化幾何（四段分支：兩直線 + 兩半圓，以周長比例確定各點）是一個典型的「數學驅動形狀繪製」技巧，完全不需要固定座標陣列，且僅靠 `kSportsTrackHalfLen` / `kSportsTrackR` 兩個常數與 `Chapter3Quest.h` / `WorldSportsLap.cpp` 完全共享跑道幾何，確保地面跑道和 Model 計算的環帶範圍在空間上對應一致。「已走過點消除」是純粹的視覺動態效果，完全由 Model 的 `SportsLapProgress()` 驅動，View 無任何狀態，符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/world/SportsLapTrack.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/world/SportsLapTrack.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
