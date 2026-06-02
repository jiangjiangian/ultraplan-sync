---
id: file:src/game/entities/EnergyDrink.cpp
type: source
path: src/game/entities/EnergyDrink.cpp
domain: game
bucket: entities
loc: 14
classes: []
sources: ["src/game/entities/EnergyDrink.cpp"]
---
# `EnergyDrink.cpp`

> **一句定位**：能量飲料消耗品的效果覆寫——使用時加業力並烘乾固定量的雨量。

## 職責

`EnergyDrink::Consume(Player*)` 是 `ConsumableItem` Template Method 鏈的末端覆寫，在玩家使用能量飲料時執行。

空 player 守衛後，呼叫 `player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief)` 同時加業力並減少固定量雨量（`kKarmaBonus` / `kRainRelief` 定義於 `EnergyDrink.h`）。設 `isActive_ = false` 標記待幀末清除。發布 `ShowMessage`（「喝完飲料，精神好多了，淋到的雨也擦乾了一些。」）。

此實作與 `ApplyConsumableEffect("EnergyDrink")` 保持一致——兩條使用路徑（地面拾取 vs 背包使用）由測試共同固定。

## 關鍵內容（類別 / 函式 / 資料）

- `EnergyDrink::Consume(Player*)`：null 守衛 + 業力加成 + 雨量減免 + 失活 + ShowMessage。
- `kKarmaBonus` / `kRainRelief`：定義於 `EnergyDrink.h` 的常數（效果量）。
- `Player::AddKarma` / `Player::DrainRainBy`：流式 API（回傳 `Player&`），一行完成兩個操作。

## 相依與在架構中的位置

- **#include（往外）**：`EnergyDrink.h`、`Player.h`（`AddKarma` / `DrainRainBy`）、`EventBus.h` / `EventSink.h`（ShowMessage）。
- **被誰使用（往內）**：—（葉節點；由 `ConsumableItem` 的多型 dispatch 呼叫）。
- **繼承 / 實作 / 體現**：`EnergyDrink` → `ConsumableItem`；`Consume` 覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：在 Collision 或背包 E 鍵路徑觸發，Sweep 階段清除。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md)：`ConsumableItem::OnPickup` 定義骨架（開門、減庫存、呼叫 `Consume`），`EnergyDrink::Consume` 填入具體效果，使不同消耗品共享拾取/使用流程而各自有不同後果。[Observer](../concepts/pat-observer.md)：`ShowMessage` 事件解耦 UI 提示與實體邏輯。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/EnergyDrink.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/EnergyDrink.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [Observer](../concepts/pat-observer.md)
