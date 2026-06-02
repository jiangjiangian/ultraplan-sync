---
id: file:include/ui/hud/RainVignette.h
type: header
path: include/ui/hud/RainVignette.h
domain: ui
bucket: hud
loc: 31
classes: []
sources: ["include/ui/hud/RainVignette.h"]
---
# `RainVignette.h`

> **一句定位**：雨壓力暗角的渲染函式聲明——由玩家 `RainMeter` 驅動的分兩段螢幕邊緣變暗，以四條邊框帶實作，從 `View::RenderHud` 抽出。

## 職責

`DrawRainVignette` 在玩家雨量計達到特定閾值時，沿螢幕四個邊框繪製漸層暗角，給予玩家「正在承受雨壓力」的視覺回饋。

兩段強度對應：`rm >= 60` 時微弱暗角，`rm >= 85` 時較強暗角（與 `RainHud.h` 的文字前綴分界一致，確保文字通道與視覺通道在相同壓力等級觸發）。無 Player 或讀數低於 60 時提前返回，每幀呼叫皆安全（廉價的空操作）。

實作以四條邊框帶（非整張全螢幕貼圖）繪製：便宜、無逐幀配置（不上傳 GPU）、具決定性且可無頭 spy 測試。所有繪製透過注入的 `IRenderer`，不直接呼叫 raylib。

純渲染（MVC）：以 `const World&` 唯讀存取（僅讀取 `GetPlayer()->GetRainMeter()`），絕不修改 World。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawRainVignette(r, world, screenW, screenH)`：
  - `r`：`IRenderer&`，繪製介面，不直接呼叫 raylib。
  - `world`：`const World&`，唯讀，僅讀 `GetPlayer()->GetRainMeter()`。
  - 無 Player 或 `rm < 60` → 提前返回（廉價空操作）。
  - `rm >= 60`：微弱暗角；`rm >= 85`：較強暗角；四條邊框帶繪製。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 include；`World` 與 `IRenderer` 以前向宣告引入）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderHud` 呼叫 `DrawRainVignette`）；`src/ui/hud/RainVignette.cpp`（`DrawRainVignette` 的實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 / hud bucket，在 `RenderHud` 階段（螢幕座標）每幀呼叫，純渲染。

## OO 概念與設計重點

本標頭與 `RainHud.h` 一起形成「雨壓力 UX 設計的兩層」：文字前綴通道（色弱備援）加上視覺暗角通道，兩者在相同的 60/85 閾值觸發，確保視覺與文字資訊一致（**無障礙設計的一致性**）。透過 `IRenderer` 注入體現了 [arch-dip-renderer](../concepts/arch-dip-renderer.md)；四條邊框帶而非全螢幕貼圖的「便宜、無配置」設計是對效能與可測性的刻意選擇。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/hud/RainVignette.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/RainVignette.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
