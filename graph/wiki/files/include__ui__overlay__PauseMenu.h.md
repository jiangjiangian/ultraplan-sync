---
id: "file:include/ui/overlay/PauseMenu.h"
type: header
path: include/ui/overlay/PauseMenu.h
domain: ui
bucket: overlay
loc: 33
classes: []
sources: ["include/ui/overlay/PauseMenu.h"]
---
# `PauseMenu.h`

> **一句定位**：宣告繪製遊戲內暫停選單疊層（全螢幕變暗＋置中面板＋6 個選項）的純渲染自由函式。

## 職責

此標頭宣告 `DrawPauseMenu` 自由函式，在玩家按 M 開啟暫停時於螢幕上渲染一個全螢幕半透明遮罩、置中面板，以及六列選項：「繼續」「說明」「減少動畫」「擴大目標」「重新開始」「離開」，底部附一條鍵盤說明帶。

從 `View::RenderOverlays` 抽出（SRP）。函式以 `const World&` 唯讀取得四個狀態：`MenuOpen`、`MenuCursor`（高亮列游標）、`ReducedMotion`、`LargeTargets`（後兩者各為一個切換列），作為純反應式渲染，View 不自行保留 UI 狀態。

`MenuOpen` 為 false 時為空操作（每幀呼叫皆安全）；所有繪製經 `IRenderer` 完成，具決定性，可無頭 spy 測試。`screenW` / `screenH` 用以計算遮罩尺寸與面板置中位置。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawPauseMenu(IRenderer& r, const World& world, float screenW, float screenH)` — 全螢幕變暗（`screenW×screenH` 半透明矩形）＋置中面板＋六列選單＋底部鍵盤說明；讀取 `World::MenuOpen/MenuCursor/ReducedMotion/LargeTargets`，MenuOpen 為假時提前返回。

## 相依與在架構中的位置
- **#include（往外）**：僅依賴前向宣告 `World` 與 `IRenderer`（無實際 include 解析）
- **被誰使用（往內）**：`src/ui/View.cpp`、`src/ui/overlay/PauseMenu.cpp`
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；於 `RenderOverlays` 階段呼叫，純渲染，不在模擬管線中執行

## OO 概念與設計重點

同 `MenuAffordance.h`，體現 [DIP](../concepts/arch-dip-renderer.md) 與 SRP：大型 View 的疊層渲染被拆成多個各自聚焦的自由函式，每個只閱讀 `const World&` 並透過 `IRenderer` 輸出。無狀態設計使本函式可在任何時刻被呼叫而無副作用。`ReducedMotion` 與 `LargeTargets` 的無障礙設定選項在此疊層切換，使配置變更立即反映至 View。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/overlay/PauseMenu.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/PauseMenu.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
