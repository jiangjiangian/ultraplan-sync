---
id: file:include/game/entities/WaterproofSpray.h
type: header
path: include/game/entities/WaterproofSpray.h
domain: game
bucket: entities
loc: 34
classes: [WaterproofSpray]
sources: ["include/game/entities/WaterproofSpray.h"]
---
# `WaterproofSpray.h`

> **一句定位**：消耗品葉類別，專責單次最大雨量減免（-35 點），不影響業力——是背包中定位為「防水裝備」而非「善行」的道具。

## 職責

`WaterproofSpray` 是 `ConsumableItem` 的三個具體葉類別之一，定位為「純功能性防水裝備」。覆寫 `Consume(player)` 以 `DrainRainBy(kRainRelief)` 減少 35 點雨量，且明確「不影響業力」——標頭說明「它是裝備，而非善行」，與 `HotPack`（+5 業力）和 `EnergyDrink`（+3 業力）形成鮮明對比。

三種消耗品的定位差異：
- `EnergyDrink`（kPrice=40）：業力導向（+3），雨量輔助（-15），Ch2 任務媒介。
- `HotPack`（kPrice=30）：業力最強（+5），雨量中等（-25），最便宜。
- `WaterproofSpray`（kPrice=50）：純雨量（-35，最強），零業力，最貴。

此梯度設計讓玩家在有限金錢下面臨有意義的道具選擇：想要業力？選 HotPack；雨量最大減免？選 WaterproofSpray（但要花更多錢）。

注意：`WaterproofSpray` 不是 `final`（不同於 `EnergyDrink`/`HotPack`），允許未來子類別化（例如升級版防水噴霧），雖然目前無子類別。

## 關鍵內容（類別 / 函式 / 資料）

- **`static constexpr int kPrice = 50`**：攤販售價（三種中最高）。
- **`static constexpr float kRainRelief = 35.0f`**：使用時減少的雨量點數（三種中最大）。
- **`WaterproofSpray(position)`**：建構子，名稱 `"WaterproofSpray"`，售價 `kPrice`，轉交 `ConsumableItem`。
- **`void Consume(Player* player) override`**：大幅減少雨量，定義於 `.cpp`；無業力效果。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/ConsumableItem.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`、`src/game/entities/WaterproofSpray.cpp`、`src/game/quest/ItemCatalog.cpp`、`tests/entities/test_consumable.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `ConsumableItem`（Template Method 葉類別）。非 `final`。
- **每幀管線 / MVC 角色**：Model 層道具；地圖上由 `WorldSpawn` 生成，也可由攤販購買；`GameController::ApplyConsumableEffect` 消費時呼叫 `Consume()`。

## OO 概念與設計重點

`WaterproofSpray` 完整體現 [Template Method](../concepts/pat-template.md) 葉類別的最小覆寫：只需定義「Consume 做什麼」，拾取入背包、冪等守衛（`isActive_`）、攤販售賣等機制全由 `ConsumableItem` 基底定稿。三個常數（無業力常數、只有 `kPrice` 和 `kRainRelief`）明確表達道具定位為「純功能性裝備」的設計意圖。

`kRainRelief = 35.0f` 仍低於 100，維持雨量系統的有效性（單次使用不清空整條雨量條），與 HotPack 的設計原則一致。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/WaterproofSpray.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/WaterproofSpray.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
