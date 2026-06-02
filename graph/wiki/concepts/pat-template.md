---
id: pat-template
type: pattern
title: Template Method
sources: [docs/UML/7-gof.md, docs/UML/1-entities.md, docs/UML/6-sequence.md]
---

# Template Method · 設計模式 (GoF)

> TransparentUmbrella::BeClaimed 與 ConsumableItem::Consume 為純虛擬骨架，子類別填行為。

## 是什麼 / 怎麼運作

`TransparentUmbrella::BeClaimed(Player*)` 與 `ConsumableItem::Consume(Player*)` 是純虛擬的『骨架步驟』：基底定義互動的固定流程（偵測 → 入袋/撐傘 → 廣播事件），把『被拾取後究竟發生什麼』留給子類別覆寫。4 把傘給 4 種後果（真傘完成章節、詛咒傘扣業力…），3 種消耗品給 3 種使用效果。基底不認得任何具體子類別。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/entities/ConsumableItem.h` | `ConsumableItem` | [node](../../index.html#node=file:include/game/entities/ConsumableItem.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/ConsumableItem.h) |
| `include/game/entities/TransparentUmbrella.h` | `TransparentUmbrella` | [node](../../index.html#node=file:include/game/entities/TransparentUmbrella.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/TransparentUmbrella.h) |

## 相關概念

[Factory Method](pat-factory.md) · [Observer](pat-observer.md)

## 來源（設計文件）

[`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md) · [`docs/UML/1-entities.md`](../../../docs/UML/1-entities.md) · [`docs/UML/6-sequence.md`](../../../docs/UML/6-sequence.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=pat-template)