---
id: file:src/game/entities/WaterproofSpray.cpp
type: source
path: src/game/entities/WaterproofSpray.cpp
domain: game
bucket: entities
loc: 15
classes: []
sources: ["src/game/entities/WaterproofSpray.cpp"]
---
# `WaterproofSpray.cpp`

> **一句定位**：防水噴霧消耗品的效果覆寫——使用時固定減少 35 點雨量，不影響業力。

## 職責

`WaterproofSpray::Consume(Player*)` 是 `ConsumableItem` Template Method 鏈中三種消耗品中雨量減免量最大的版本，且唯一不含業力獎勵。

空 player 守衛後，呼叫 `player->DrainRainBy(kRainRelief)` 減少固定量雨量（`kRainRelief` 定義於 `WaterproofSpray.h`）。說明中指出它是「裝備而非善行」——防水噴霧是純功能性道具，不代表道德選擇，故刻意不呼叫 `AddKarma`。設 `isActive_=false`，發布 `ShowMessage`（「噴了防水噴霧，雨水大半都被彈開了。」）。

與 `ApplyConsumableEffect("WaterproofSpray")` 保持數值一致。

## 關鍵內容（類別 / 函式 / 資料）

- `WaterproofSpray::Consume(Player*)`：null 守衛 + 固定雨量減免（無業力）+ 失活 + ShowMessage。
- `kRainRelief`：定義於 `WaterproofSpray.h`（= 35.0f，三種消耗品中最大）。

## 相依與在架構中的位置

- **#include（往外）**：`WaterproofSpray.h`、`Player.h`（`DrainRainBy`）、`EventBus.h` / `EventSink.h`。
- **被誰使用（往內）**：—（葉節點；消耗品多型 dispatch）。
- **繼承 / 實作 / 體現**：`WaterproofSpray` → `ConsumableItem`；覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：Collision 或背包路徑觸發，Sweep 清除。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md)：與 `EnergyDrink` / `HotPack` 共享消耗品骨架，但 `WaterproofSpray` 是三種消耗品中唯一不給業力的，體現了 Template Method 允許子類別在「不影響共同流程」的前提下實現差異化效果。「裝備而非善行」的設計說明是一個明確的遊戲設計立場，被直接寫入行內說明。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/WaterproofSpray.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/WaterproofSpray.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
