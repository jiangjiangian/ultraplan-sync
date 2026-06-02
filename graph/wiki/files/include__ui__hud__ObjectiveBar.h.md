---
id: file:include/ui/hud/ObjectiveBar.h
type: header
path: include/ui/hud/ObjectiveBar.h
domain: ui
bucket: hud
loc: 33
classes: []
sources: ["include/ui/hud/ObjectiveBar.h"]
---
# `ObjectiveBar.h`

> **一句定位**：任務目標列的渲染函式聲明——從 `View::RenderHud` 抽出的獨立模組，在頂部中央繪製當前章節目標文字，不與左側狀態面板重疊。

## 職責

`DrawObjectiveBar` 是從 `View::RenderHud` 抽出的 HUD 子元件，負責在畫面頂部中央（位於左側狀態面板下方，約 y132 以內）繪製當前章節目標一行文字，並以精確量測的寬度加上面板底色。

函式設計為純渲染函式（MVC 純度）：以 `const World&` 唯讀存取（不修改），為 `CurrentObjective(st, *world.GetPlayer())` 的純函式，每幀呼叫皆安全。無 Player 或目標字串為空時即空操作（提前返回）。面板寬度由 `TextBuilder::Measure()` 精確量測，隨目標文字伸縮，避免固定寬度在短文字時顯得過寬或長文字時截斷。

自 `View::RenderHud` 抽出的動機是關注點分離：`RenderHud` 本身已包含多個 HUD 元件（狀態面板、vignette、圈速環），將任務目標列拆到獨立模組使各模組可單獨審閱與測試。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawObjectiveBar(r, world, st, screenW, screenH)`：
  - `r`：`IRenderer&`，所有繪製注入，不直接呼叫 raylib。
  - `world`：`const World&`，唯讀存取 `GetPlayer()` 取當前目標。
  - `st`：`SemesterState`，傳入 `CurrentObjective(st, player)` 確定當前章節目標字串。
  - `screenW`、`screenH`：螢幕尺寸，用於置中計算。
  - 邏輯：無 Player 或目標字串為空 → 提前返回；否則量測文字寬度、繪製面板底色、繪製目標文字。

## 相依與在架構中的位置

- **#include（往外）**：`game/state/SemesterState.h`（`SemesterState` 參數型別）。
- **被誰使用（往內）**：`src/ui/View.cpp`（`RenderHud` 呼叫 `DrawObjectiveBar`）；`src/ui/hud/ObjectiveBar.cpp`（`DrawObjectiveBar` 的實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層 / hud bucket，在 `RenderHud` 階段（螢幕座標）每幀呼叫，純渲染，以 `const` 讀取 World。

## OO 概念與設計重點

本標頭體現了 **單一職責（SRP）**：把「繪製任務目標列」這一職責從 `View::RenderHud` 中抽出成獨立函式，使 `RenderHud` 可以呼叫多個小而清晰的子元件，而非一個龐大的方法。透過 `IRenderer` 注入（[arch-dip-renderer](../concepts/arch-dip-renderer.md)），讓此 HUD 元件在無頭流程中可被 spy 測試。純函式設計（無狀態，`const World&` 唯讀）是 View 層「只渲染不修改」原則的直接體現。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/hud/ObjectiveBar.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/hud/ObjectiveBar.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md) · [MVC](../concepts/arch-mvc.md)
