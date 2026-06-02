---
id: file:src/README.md
type: doc
path: src/README.md
domain: root
bucket: 
loc: 51
classes: []
sources: ["src/README.md"]
---
# `README.md`（src/ 樹說明）

> **一句定位**：`src/` 實作檔樹的領域分層導覽——與 `include/` 平行鏡射的 app/engine/game/ui 四層。

## 職責
`src/` 樹的門面文件，說明 `.cpp` 實作如何依 **app / engine / game / ui** 四領域分層（與 `include/`
平行鏡射），相依方向單向 `app → game / ui → engine`。並說明為何 `src/` 僅 80 檔少於 `include/`：
engine 的 math / render 包裝與 `game/gfx` 多為 header-only，只有「需存放狀態或非平凡邏輯」者才有對應 `.cpp`。

## 內容（這份說明涵蓋什麼）
- 四領域實作職責：`app`（含 `main.cpp` composition root，刻意置於 app 根）· `engine`（有狀態 / 需連結
  Raylib 者：audio/events/platform/render 的 `RaylibRenderer`）· `game`(50) · `ui`(15)。
- 完整目錄樹與各 bucket 的 `.cpp` 計數。
- 註記 `state/EndingGate` 對應狀態機 **四結局**。

## 相依與在架構中的位置
純文件，無程式相依。對應 [MVC 架構](../concepts/arch-mvc.md)；與 [`include/` 樹說明](include__README.md.md) 平行鏡射。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/README.md) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/README.md) · [← 全檔索引](../files-index.md) · 相關：[app 領域](../domains/app.md) · [game 領域](../domains/game.md) · [ui 領域](../domains/ui.md)
