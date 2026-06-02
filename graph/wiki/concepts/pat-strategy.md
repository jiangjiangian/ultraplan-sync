---
id: pat-strategy
type: pattern
title: Strategy / Pipeline
sources: [docs/UML/3-mvc-isystem.md, docs/UML/6-sequence.md, docs/UML/7-gof.md]
---

# Strategy / Pipeline · 設計模式 (GoF)

> ISystem::Run 的五個 stage（Survival/Movement/Collision/Spawn/Sweep）由 Controller 依序執行。

## 是什麼 / 怎麼運作

原本約 793 行的 `GameController::Update()` god-method 被拆成一條 `ISystem` 管線：每個 stage（`SurvivalSystem`、`MovementSystem`、`CollisionSystem`、`SpawnSystem`、`SweepSystem`）只做一件事、實作同一個 `Run(SimContext&, dt)`，由 Controller 以固定順序執行、透過 `SimContext` 串接。stage 可組合、可重排、可單獨測試，也是 Assignment #6 生存玩法（敵人 spawner、碰撞傷害）的直接擴充點。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/controller/SimSystem.h` | `SimContext`, `ISystem`, `SurvivalSystem`, `MovementSystem`, `CollisionSystem`, `SpawnSystem`, `SweepSystem` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/SimSystem.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/SimSystem.h) |

## 相關概念

[ISystem 模擬管線](arch-isystem.md) · [State](pat-state.md) · [角色介面（ISP）](oo-isp-roles.md)

## 來源（設計文件）

[`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md) · [`docs/UML/6-sequence.md`](../../../docs/UML/6-sequence.md) · [`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=pat-strategy)