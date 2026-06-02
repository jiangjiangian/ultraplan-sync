---
id: file:include/ui/hud/StatusPanel.h
type: header
path: include/ui/hud/StatusPanel.h
domain: ui
bucket: hud
loc: 30
classes: []
sources: ["include/ui/hud/StatusPanel.h"]
---
# `StatusPanel.h`

> **一句定位**：左上角 HUD 狀態面板的渲染函式聲明——半透明黑底 + 最多 7 列文字（操作提示、業力/傘、金幣、建築內外、章節名、雨量），從 `View::RenderHud` 抽出。

## 職責

`DrawStatusPanel` 繪製左上角的主 HUD 面板，包含一塊半透明黑底矩形加上最多 7 列文字：WASD 移動提示、Tab/M 操作提示、業力＋雨傘狀態、金幣數量、Inside 標記（身處建築內時顯示）、章節名、雨量讀數（含 `RainHud.h` 的色弱備援前綴與三段顏色斜坡）。

寬度由實際 UTF-8 碼點數估算（CJK 字符視為寬度 ×2），使黑底矩形貼齊最寬一列，外觀整齊。固定錨定在螢幕左上角 `(10, 10)` 偏移，呼叫端無需傳入位置。

純渲染（MVC）：以 `const World&`（及其 Player）唯讀存取所有顯示數值，絕不修改狀態。所有繪製透過注入的 `IRenderer`，不呼叫 raylib，具決定性且可由無頭流程逐位元驗證。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawStatusPanel(r, world)`：
  - `r`：`IRenderer&`，繪製介面，不直接呼叫 raylib。
  - `world`：`const World&`，唯讀，讀取 `GetPlayer()` 取業力/傘/金幣/雨量、`CurrentBuildingName()` 取建築名、`Semester()` 取章節名。
  - 固定位置 `(10, 10)`，寬度自適應最寬列。
  - 7 列：WASD 提示 / Tab·M 提示 / 業力+傘 / 金幣 / Inside / 章節 / 雨量。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`World` 與 `IRenderer` 以前向宣告引入）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderHud` 呼叫 `DrawStatusPanel`）；`src/ui/hud/StatusPanel.cpp`（`DrawStatusPanel` 的實作，其中 include `RainHud.h` 取前綴函式）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 / hud bucket，在 `RenderHud` 階段（螢幕座標）每幀呼叫，純渲染。

## OO 概念與設計重點

`DrawStatusPanel` 是「把 `View::RenderHud` 按關注點拆分成獨立模組」的主力模組：狀態面板的邏輯（7 列布局、寬度估算、顏色判斷）被完整封裝在這一個函式中，`View` 只需一次呼叫即可繪製左上角 HUD。`IRenderer` 注入遵循 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。雨量讀數的前綴由 `RainHud.h` 的 `RainTierPrefix` 提供，展現了 ui 層元件之間的協作而非重複實作色弱備援邏輯。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/hud/StatusPanel.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/StatusPanel.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
