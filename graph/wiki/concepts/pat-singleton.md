---
id: pat-singleton
type: pattern
title: Singleton
sources: [docs/UML/7-gof.md, docs/UML/3-mvc-isystem.md]
---

# Singleton · 設計模式 (GoF)

> EventBus::Instance() 全域事件匯流排（shared_mutex 僅護 handler list）。

## 是什麼 / 怎麼運作

`EventBus::Instance()` 提供全域唯一的事件匯流排，讓散落各處的 publisher / subscriber 共用同一條通道而不必彼此持有參照。內部以 `shared_mutex` 保護 handler list（注意：只護 list，handler 本體仍須單執行緒呼叫，因 GL 單執行緒——見技術債 H1）。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/events/EventBus.h` | `Event`, `EventBus`, `Subscription`, `Slot` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/events/EventBus.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventBus.h) |

## 相關概念

[Observer](pat-observer.md)

## 來源（設計文件）

[`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md) · [`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=pat-singleton)