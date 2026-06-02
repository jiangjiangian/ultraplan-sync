---
id: file:src/ui/overlay/PauseMenu.cpp
type: source
path: src/ui/overlay/PauseMenu.cpp
domain: ui
bucket: overlay
loc: 80
classes: []
sources: ["src/ui/overlay/PauseMenu.cpp"]
---
# `PauseMenu.cpp`

> **一句定位**：遊戲內暫停選單疊層——MenuOpen 時顯示六個選項（繼續 / 說明 / 減少動畫 / 擴大目標 / 重新開始 / 離開），游標所在列以金色插字符高亮。

## 職責

此檔屬於 ui / overlay 層，只實作 `DrawPauseMenu` 一個函式。以 `world.MenuOpen()` 守衛（選單未開啟時直接返回），畫全螢幕半透明暗化層（alpha 150），再畫置中面板（340×330，`Color{20,22,30,230}`），繪製「遊戲選單」金色標題。

六個選項依序為：`"繼續"` / `"說明"` / `"減少動畫"` / `"擴大目標"` / `"重新開始"` / `"離開"`，以 `World::kMenuItemCount=6` 迴圈。可切換的列（索引 2 = 減少動畫、索引 3 = 擴大目標）在標籤後附加 `[開]` / `[關]` 狀態字串（分別讀取 `world.ReducedMotion()` / `world.LargeTargets()`）。游標所在列加上 `"> "` 前綴並以金色（`Color{255,153,0,255}`）顯示，其餘白色。最下方導覽提示文字改用 `Color{180,180,180,255}`（原為 DarkGray，對比不足）以達到 WCAG AA 以上。

破壞性選項（重新開始 / 離開）排在最後，與游標預設的「繼續」（索引 0）距離最遠，防止誤觸。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawPauseMenu(IRenderer&, World&, float W, float H)` — 唯一公開函式。
- `kLabels[World::kMenuItemCount]` — 六個選項標籤的 static 字串表。
- `kPanelW`=340、`kPanelH`=330、`kFirstY`=78、`kRowH`=40 — 面板版面常數。
- `world.MenuCursor()` — 當前高亮列（由 GameController 管理）。
- `world.ReducedMotion()` / `world.LargeTargets()` — 可切換旗標的當前狀態。

## 相依與在架構中的位置

- **#include（往外）**：`PauseMenu.h`、`World.h`（`MenuOpen`/`MenuCursor`/`ReducedMotion`/`LargeTargets`/`kMenuItemCount`）、`IRenderer.h`、`TextBuilder.h`、`Color.h`/`Rect.h`/`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderOverlays` 每幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層疊層；最後繪製（位於世界 / HUD / 對話 / 背包之上）；實際選項效果由 GameController 在 Update 中處理（View 不含玩法邏輯）。

## OO 概念與設計重點

可切換列（無障礙設定）直接在 `DrawPauseMenu` 內讀取 Model 狀態並附加 `[開]/[關]`，使面板始終反映當前設定值，無需額外 UI 狀態，符合「反應式渲染」的 MVC 純度。導覽提示文字的對比修正（DarkGray→`{180,180,180}`）體現了無障礙設計的持續改善。破壞性選項排在末尾是 UX 防禦設計，與結局底部選單的「回首頁/重新開始最後」同一思路。符合 [MVC](../concepts/arch-mvc.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/overlay/PauseMenu.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/overlay/PauseMenu.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
