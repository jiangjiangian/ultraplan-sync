---
id: arch-mvc
type: architecture
title: MVC 核心
sources: [docs/UML/0-layer-map.md, docs/UML/3-mvc-isystem.md, docs/UML/8-solid.md]
---

# MVC 核心 · 架構元件

> World＝純資料 Model、View＝只讀模型繪圖、GameController＝收輸入＋跑模擬＋接事件。

## 是什麼 / 怎麼運作

三權分立：`World` 是**純資料 Model**（擁有所有 GameObject、學期 FSM、碰撞遮罩、對話/HUD/背包狀態），不認得 raylib、不讀輸入；`View` **只讀** `const World&` 把畫面畫出來；`GameController` 收輸入、跑 `ISystem` 模擬、接 `EventBus` 事件、改 World。`main.cpp` 是薄薄的組裝根。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/controller/GameController.h` | `GameController` | [node](../../index.html#node=file:include/game/controller/GameController.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameController.h) |
| `include/game/world/World.h` | `World` | [node](../../index.html#node=file:include/game/world/World.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/World.h) |
| `include/ui/View.h` | `View`, `BuildingSprite`, `DrawRef`, `DecorationSprite` | [node](../../index.html#node=file:include/ui/View.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/View.h) |

## 相關概念

[ISystem 模擬管線](arch-isystem.md) · [DIP：IRenderer](arch-dip-renderer.md) · [Observer](pat-observer.md)

## 來源（設計文件）

[`docs/UML/0-layer-map.md`](../../../docs/UML/0-layer-map.md) · [`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=arch-mvc)