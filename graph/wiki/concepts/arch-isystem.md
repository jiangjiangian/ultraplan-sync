---
id: arch-isystem
type: architecture
title: ISystem 模擬管線
sources: [docs/UML/3-mvc-isystem.md, docs/UML/6-sequence.md]
---

# ISystem 模擬管線 · 架構元件

> 每幀邏輯拆成可插拔的 ISystem stage，透過 SimContext 串接；god-method 解體。

## 是什麼 / 怎麼運作

每幀的 model 推進是一條有序的 `ISystem` 管線，透過 `SimContext`（world、世界尺寸、本幀 collider、上一幀玩家座標）串接。鐵律：`ISystem` 只動 model（`World&`/`Player&`），不讀輸入、不呼叫 raylib、不繪圖。順序：Survival → Movement → Collision → Spawn →（互動/結局判定）→ Sweep。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/controller/SimSystem.h` | `SimContext`, `ISystem`, `SurvivalSystem`, `MovementSystem`, `CollisionSystem`, `SpawnSystem`, `SweepSystem` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/SimSystem.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SimSystem.h) |

## 相關概念

[Strategy / Pipeline](pat-strategy.md) · [MVC 核心](arch-mvc.md) · [RAII / 記憶體安全](oo-raii.md)

## 來源（設計文件）

[`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md) · [`docs/UML/6-sequence.md`](../../../docs/UML/6-sequence.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=arch-isystem)