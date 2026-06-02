---
id: arch-dip-renderer
type: architecture
title: DIP：IRenderer
sources: [docs/UML/4-gfx.md, docs/UML/8-solid.md]
---

# DIP：IRenderer · 架構元件

> 所有 raylib Draw* 關在 IRenderer 後；Model 端只認 IRenderer&，永不 include raylib（架構紅線）。

## 是什麼 / 怎麼運作

所有 raylib `::Draw*` 都關在 `IRenderer` 介面後，`RaylibRenderer` 是唯一具體實作。Model 端寫 `Render(IRenderer&)`，永遠不 `#include` raylib（架構紅線＃2）——這就是依賴反轉：高層 Model 不依賴低層繪圖細節，兩者都依賴抽象。材質經 process-lifetime 的 `TextureCache` 只上傳一次。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/render/IRenderer.h` | `IRenderer` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/IRenderer.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/IRenderer.h) |
| `include/engine/render/RaylibRenderer.h` | `RaylibRenderer` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/RaylibRenderer.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/RaylibRenderer.h) |

## 相關概念

[MVC 核心](arch-mvc.md) · [RAII / 記憶體安全](oo-raii.md)

## 來源（設計文件）

[`docs/UML/4-gfx.md`](../../../docs/UML/4-gfx.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=arch-dip-renderer)