---
id: file:include/README.md
type: doc
path: include/README.md
domain: root
bucket: 
loc: 55
classes: []
sources: ["include/README.md"]
---
# `README.md`（include/ 樹說明）

> **一句定位**：`include/` 標頭檔樹的領域分層導覽——說明 app/engine/game/ui 四層與單向相依方向。

## 職責
`include/` 樹的門面文件，說明標頭檔如何依 **app / engine / game / ui** 四領域分層，相依方向單向
`app → game / ui → engine`（engine 不反向相依）。並解釋為何 `include/`（144 檔）多於 `src/`（80）：
engine 的 math / render 包裝與 `game/gfx` 多為 header-only，故無對應 `.cpp`。

## 內容（這份說明涵蓋什麼）
- 四領域職責界定：`app`＝組裝層（composition root）與場景切換；`game`/`ui`＝遊戲層（建於 engine 之上、
  彼此經 EventBus / View 介面解耦）；`engine`＝與本遊戲無關的可重用引擎層。
- 完整目錄樹：app(7) · engine(27：audio/core/events/input/math/platform/render) ·
  game(89：controller/dialog/entities/gfx/quest/state/vendor/world) · ui(21：扁平視圖＋hud/overlay/world)。
- 註記 `state/EndingGate` 對應狀態機 **四結局** Ending_A/B/C/D。

## 相依與在架構中的位置
純文件，無程式相依。對應 [MVC 架構](../concepts/arch-mvc.md) 的領域分層理念；與 [`src/` 樹說明](src__README.md.md) 平行鏡射。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/README.md) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/README.md) · [← 全檔索引](../files-index.md) · 相關：[app 領域](../domains/app.md) · [engine 領域](../domains/engine.md)
