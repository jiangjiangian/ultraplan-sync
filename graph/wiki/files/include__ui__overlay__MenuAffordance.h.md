---
id: "file:include/ui/overlay/MenuAffordance.h"
type: header
path: include/ui/overlay/MenuAffordance.h
domain: ui
bucket: overlay
loc: 33
classes: []
sources: ["include/ui/overlay/MenuAffordance.h"]
---
# `MenuAffordance.h`

> **一句定位**：在遊戲右上角繪製「M 選單」操作提示標籤的純渲染自由函式宣告。

## 職責

此標頭宣告一個自由函式 `DrawMenuAffordance`，用以在螢幕右上角繪製一個小型面板標籤，提醒玩家如何喚出暫停選單（按 M 鍵）。設計上刻意簡單：無類別、無狀態、只有一個函式。

從 `View::RenderOverlays` 抽出，讓疊層渲染各自聚焦在一個職責（SRP）。當 `World::MenuOpen` 為 true 時（選單已開啟，由 `PauseMenu` 取代此標籤），函式提前返回，為空操作。

這是 View 層（`ui/overlay/`）的純渲染元件：以 `const World&` 唯讀讀取 `MenuOpen` 狀態，絕不修改模型。所有繪製透過注入的 `IRenderer` 完成，因此具備可測試性（無頭 spy 測試）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawMenuAffordance(IRenderer& r, const World& world, float screenW, float screenH)` — 在右上角（內縮 6 px）繪製面板底色＋「M 選單」文字標籤；`MenuOpen` 為真時提前返回。

## 相依與在架構中的位置
- **#include（往外）**：僅依賴前向宣告 `World` 與 `IRenderer`（無實際 include 解析）
- **被誰使用（往內）**：`src/ui/View.cpp`、`src/ui/overlay/MenuAffordance.cpp`
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；於 `RenderOverlays` 階段呼叫，純渲染，不參與 Survival→…→Sweep 模擬管線

## OO 概念與設計重點

此檔案體現 [ISP](../concepts/oo-isp-roles.md) 精神：不藉由大型渲染類別的成員，而以自由函式將小型 UI 職責拆開。[DIP](../concepts/arch-dip-renderer.md) 原則透過注入 `IRenderer&` 而非直接呼叫 raylib 得以落實，使渲染決定性且可替換。無狀態的純函式風格確保每幀呼叫皆安全。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/ui/overlay/MenuAffordance.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/overlay/MenuAffordance.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
