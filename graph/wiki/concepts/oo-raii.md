---
id: oo-raii
type: principle
title: RAII / 記憶體安全
sources: [docs/UML/4-gfx.md, docs/UML/8-solid.md, docs/UML/6-sequence.md]
---

# RAII / 記憶體安全 · OO 原則 / 技法

> 物件以 unique_ptr 持有；Texture/Font/Subscription 皆 RAII；移除採 isActive 旗標 + 幀末 Sweep（mark-then-sweep）。

## 是什麼 / 怎麼運作

資源生命週期一律綁在物件上：GameObject 以 `unique_ptr` 持有；`Texture`/`Font` 是 move-only RAII 包裝、GL 資源在 `~Window`/`CloseWindow` 前顯式釋放；`EventBus::Subscription` 是 RAII 退訂 token。物件移除**不在迭代中 `delete`**——改標記 `isActive_=false`，由 `SweepSystem`→`World::Sweep()` 於幀末 mark-then-sweep 統一 erase-remove（`objects_.front()` 恆為 Player）。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/events/EventBus.h` | `Event`, `EventBus`, `Subscription`, `Slot` | [node](../../index.html#node=file:include/engine/events/EventBus.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventBus.h) |
| `include/engine/render/Texture.h` | `Texture`, `TextureCache` | [node](../../index.html#node=file:include/engine/render/Texture.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Texture.h) |

## 相關概念

[Observer](pat-observer.md) · [ISystem 模擬管線](arch-isystem.md)

## 來源（設計文件）

[`docs/UML/4-gfx.md`](../../../docs/UML/4-gfx.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md) · [`docs/UML/6-sequence.md`](../../../docs/UML/6-sequence.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=oo-raii)