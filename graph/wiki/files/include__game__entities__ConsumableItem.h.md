---
id: file:include/game/entities/ConsumableItem.h
type: header
path: include/game/entities/ConsumableItem.h
domain: game
bucket: entities
loc: 79
classes: [ConsumableItem]
sources: ["include/game/entities/ConsumableItem.h"]
---
# `ConsumableItem.h`

> **一句定位**：消耗品拾取物的抽象中間層（Template Method 骨架），定稿「拾取入背包」流程，留純虛擬 `Consume()` 給 `EnergyDrink`／`HotPack`／`WaterproofSpray` 覆寫效果。

## 職責

`ConsumableItem` 是 `Item` 與三個具體消耗品葉類別之間的中介抽象層，對應雨傘樹中 `TransparentUmbrella` 的角色——差別在多型動詞是 `Consume()` 而非 `BeClaimed()`。

拾取流程（`Collect`）在本層定稿：`Interact` 與 `OnPickup` 兩條入口皆轉呼叫私有 `Collect(player)`，後者以 `isActive_` 守衛冪等——已失效的世界物件不會重複計入背包。成功收取後呼叫 `player->AddConsumable(GetName())` 並設 `isActive_ = false`（幀末 `World::Sweep()` 移除），完成一次性拾取。

「效果」刻意「延後」到玩家從背包使用時才套用：拾取只把 `itemId` 計入 `Player::consumables_`，`GameController` 導向 `ApplyConsumableEffect` 時才呼叫葉類別的 `Consume(player)`，由此觸發業力增減、雨量減免、`ShowMessage` 等效果。

ISP 設計：`ConsumableItem` 僅扮演 `IInteractable`，不實作 `IUpdatable`（消耗品無逐幀更新需求）與 `IDrawable`（繪製由 View 負責），故管線不為它執行空覆寫。`WithRoles<ConsumableItem, Item>` 以中間層為鍵，使三個葉類別的 `static_cast<ConsumableItem*>` 合法。

## 關鍵內容（類別 / 函式 / 資料）

- **`ConsumableItem(position, name, price)`**：建構子，碰撞盒固定 16×16，轉交 `WithRoles(position, Rect{...16x16...}, name)` 與 `price_`。
- **`void Interact(Player*) override`**：呼叫 `Collect(initiator)`——E 互動路徑入口。
- **`void OnPickup(Player*) override`**：呼叫 `Collect(player)`——碰撞拾取路徑入口。
- **`virtual void Consume(Player*) = 0`**：純虛擬的效果鉤子，葉類別覆寫以施放業力／雨量效果。
- **`int GetPrice() const noexcept`**：取得攤販售價。
- **`int price_`**（protected）：攤販售價。
- **`Collect(Player*)`**（private）：共用拾取邏輯；以 `isActive_` 保證冪等。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/Item.h`（基底，提供名稱、`isPickable_`、`OnPickup` 鉤子）、`include/game/entities/Player.h`（`AddConsumable` 接口）。
- **被誰使用（往內）**：`include/game/entities/EnergyDrink.h`、`include/game/entities/HotPack.h`、`include/game/entities/WaterproofSpray.h`（三個葉類別的基底）；`tests/entities/test_consumable.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `WithRoles<ConsumableItem, Item>` 與 `IInteractable`（來自 Roles.h）；被三個消耗品葉類別繼承。體現 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：Model 層道具物件。幀末 `Sweep()` 移除失效實例；背包 UI 讀取 `Player::Consumables()` 渲染；`GameController` 的 `ApplyConsumableEffect` 呼叫 `Consume()`。

## OO 概念與設計重點

本類別是教科書式的 [Template Method](../concepts/pat-template.md)：`Collect` 固定「拾取入背包」的不變步驟，`Consume` 留為子類別的可變鉤子。拾取與使用的「分離」是關鍵設計決策：玩家撿起消耗品時僅計數，效果延後到主動使用，使消耗品在背包中有意義的存在（而非立即生效消失）。

[ISP（角色介面分離）](../concepts/oo-isp-roles.md)體現在只扮演 `IInteractable`：消耗品在地圖上不需逐幀更新、也不自行繪製，強制每個角色都是真實的實作而非空覆寫。`WithRoles<ConsumableItem, Item>` 的 CRTP 鍵位設計讓葉類別免於重複宣告角色集（[CRTP](../concepts/oo-crtp.md)）。

## 連結
[🕸 圖譜節點](../../index.html#node=file:include/game/entities/ConsumableItem.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/ConsumableItem.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [ISP / Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
