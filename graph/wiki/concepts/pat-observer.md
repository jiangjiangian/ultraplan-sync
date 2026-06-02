---
id: pat-observer
type: pattern
title: Observer
sources: [docs/UML/7-gof.md, docs/UML/3-mvc-isystem.md, docs/UML/6-sequence.md]
---

# Observer · 設計模式 (GoF)

> EventBus Subscribe/Publish 解耦事件；Subscription 為 RAII 退訂 token。

## 是什麼 / 怎麼運作

`EventBus`（單例）讓事件的『發出方』與『反應方』完全解耦。Model 端（傘、Vendor、Player）只 `Publish(Event)`，不知道誰在聽；UI/HUD/Toast/harness 用 `Subscribe` / `ScopedSubscribe` 註冊 handler。事件有 `UmbrellaClaimed`、`KarmaChanged`、`ShowMessage`、`EnteredBuilding`、`PickupAcquired`。`Publish` 對 handler 的 snapshot 廣播（避免廣播中改訂閱造成迭代失效）。`ScopedSubscribe` 回傳 `Subscription`（RAII token），出作用域自動退訂。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/events/EventBus.h` | `Event`, `EventBus`, `Subscription`, `Slot` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/events/EventBus.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventBus.h) |

## 相關概念

[Singleton](pat-singleton.md) · [RAII / 記憶體安全](oo-raii.md) · [MVC 核心](arch-mvc.md)

## 來源（設計文件）

[`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md) · [`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md) · [`docs/UML/6-sequence.md`](../../../docs/UML/6-sequence.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=pat-observer)