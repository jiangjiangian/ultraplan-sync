---
id: file:include/game/entities/HotPack.h
type: header
path: include/game/entities/HotPack.h
domain: game
bucket: entities
loc: 36
classes: [HotPack]
sources: ["include/game/entities/HotPack.h"]
---
# `HotPack.h`

> **一句定位**：消耗品葉類別，使用時烘乾 25 點雨量並加 5 業力，是背包中單次雨量減免第二強的道具（僅次於防水噴霧）。

## 職責

`HotPack` 是 `ConsumableItem` 的三個具體葉類別之一，象徵讓玩家在大雨中暖和起來。覆寫 `Consume(player)` 施放效果：以 `DrainRainBy(kRainRelief)` 減少 25 點雨量，並加 `kKarmaBonus`（5）業力。

關鍵設計決策：雨量減免固定為 -25 點，而非歸零。標頭明確說明這個選擇：「使雨量這項支柱即便背包裡有消耗品仍保有意義——一個暖暖包不再抹平整條雨量條，遊戲仍可通關」。三種消耗品的減免梯度（EnergyDrink -15 < HotPack -25 < WaterproofSpray -35）形成有意義的選擇差異，而非可替換的等效道具。

`kKarmaBonus = 5` 是三種消耗品中最高的業力加成，暗示暖暖包有「施與溫暖」的道德意涵，比純功能性的飲料或噴霧更「善良」。

## 關鍵內容（類別 / 函式 / 資料）

- **`static constexpr int kPrice = 30`**：攤販售價（三種中最便宜）。
- **`static constexpr int kKarmaBonus = 5`**：使用時的業力加成（三種中最高）。
- **`static constexpr float kRainRelief = 25.0f`**：使用時減少的雨量點數。
- **`HotPack(position)`**：建構子，名稱 `"HotPack"`，售價 `kPrice`，轉交 `ConsumableItem`。
- **`void Consume(Player* player) override`**：施放效果，定義於 `.cpp`。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/ConsumableItem.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`、`src/game/entities/HotPack.cpp`、`src/game/quest/ItemCatalog.cpp`、`tests/entities/test_consumable.cpp`、`tests/entities/test_roles.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `ConsumableItem`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層道具；地圖上由 `WorldSpawn` 生成，也可由攤販購買；`GameController::ApplyConsumableEffect` 消費時呼叫 `Consume()`。

## OO 概念與設計重點

`HotPack` 與 `EnergyDrink`、`WaterproofSpray` 一同構成三個同構的消耗品葉類別，共享 [Template Method](../concepts/pat-template.md) 骨架（`ConsumableItem::Collect` 定稿拾取，各自 `Consume` 定義效果）。三個常數（`kPrice`、`kKarmaBonus`、`kRainRelief`）的設計讓攤販定價、背包 UI 數值與效果計算永不漂移，是「單一事實來源」原則的實踐。

`kRainRelief` 不等於 100（不歸零）是刻意的平衡設計，避免消耗品使雨量系統這個核心遊戲機制失去意義。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/HotPack.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/HotPack.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
