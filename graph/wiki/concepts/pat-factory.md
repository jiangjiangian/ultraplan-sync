---
id: pat-factory
type: pattern
title: Factory Method
sources: [docs/UML/7-gof.md, docs/UML/1-entities.md]
---

# Factory Method · 設計模式 (GoF)

> GameObjectFactory::Create(ObjectType) 由列舉動態產生 12 種具體 GameObject。

## 是什麼 / 怎麼運作

`GameObjectFactory::Create(ObjectType, Vec2)` 是單一進入點，依 `ObjectType` 列舉把『要生哪種物件』與『誰來 new』解耦。呼叫端（spawn 設定、`SpawnSystem`、章節 roster）只給一個列舉值與座標，工廠回傳 `unique_ptr<GameObject>`；12 種具體型別（4 把傘、3 種消耗品、Vendor、3 種金幣、Player…）對呼叫端不可見。新增一種物件＝工廠加一個 case + 列舉加一項，呼叫端不動（OCP）。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/controller/GameObjectFactory.h` | `GameObjectFactory` | [node](../../index.html#node=file:include/game/controller/GameObjectFactory.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameObjectFactory.h) |

## 相關概念

[Template Method](pat-template.md) · [角色介面（ISP）](oo-isp-roles.md) · [ISystem 模擬管線](arch-isystem.md)

## 來源（設計文件）

[`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md) · [`docs/UML/1-entities.md`](../../../docs/UML/1-entities.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=pat-factory)