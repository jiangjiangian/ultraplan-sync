---
id: file:include/ui/hud/SportsLapRing.h
type: header
path: include/ui/hud/SportsLapRing.h
domain: ui
bucket: hud
loc: 30
classes: []
sources: ["include/ui/hud/SportsLapRing.h"]
---
# `SportsLapRing.h`

> **一句定位**：操場校慶圈速進度環（HUD）的渲染函式聲明——右上角以 16 個小方塊繪製的進度環，對應世界座標的地面跑道，從 `View::RenderHud` 抽出。

## 職責

`DrawSportsLapRing` 在第三章校慶運動會期間，於畫面右上角以 16 個小方塊排成的圓形進度環顯示操場繞圈進度，是世界座標地面跑道（`SportsLapTrack`）在螢幕上的對應 HUD 元件。

函式反應式：為 `World::SportsLapActive()` 與 `World::SportsLapProgress()` 的純函式——未進行圈速（`SportsLapActive()` 為 false）時提前返回，每幀呼叫皆安全（廉價空操作）。進行中時，圓環以 16 個小實心方塊順時針填滿，填滿比例由 `SportsLapProgress()` [0,1] 決定。

刻意以 16 個小方塊繪製（而非呼叫 `renderer.DrawCircle`），原因是維持可無頭 spy 測試：`DrawCircle` 可能需要 OpenGL 上下文才能正確執行，而 `DrawRect` 在無頭 spy 替身中能完整記錄（與 `DrawQuestGiverIndicator` 同設計考量）。

純渲染（MVC）：以 `const World&` 唯讀，絕不修改 World。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawSportsLapRing(r, world, screenW, screenH)`：
  - `r`：`IRenderer&`，繪製介面。
  - `world`：`const World&`，唯讀，讀取 `SportsLapActive()` 與 `SportsLapProgress()`。
  - 未進行圈速 → 提前返回。
  - 進行中：右上角以 16 個小方塊排成環，按 `SportsLapProgress()` 比例順時針填滿。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`World` 與 `IRenderer` 以前向宣告引入）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderHud` 呼叫）；`src/ui/hud/SportsLapRing.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 / hud bucket，在 `RenderHud` 階段（螢幕座標）每幀呼叫。

## OO 概念與設計重點

本標頭是「把 `View::RenderHud` 按關注點拆分為獨立模組」的一個例子，體現 **SRP**。用 `DrawRect` 取代 `DrawCircle` 的設計選擇體現了 **可測性優先**：測試用的 spy IRenderer 可以記錄 `DrawRect` 調用，讓 HUD 環的渲染結果可以在無 GL 環境中精確驗證，對應 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。`SportsLapActive()` 的提前返回讓此函式在非第三章或已完成圈速時完全廉價，避免不必要的計算。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/hud/SportsLapRing.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/SportsLapRing.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
