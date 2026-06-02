---
id: file:src/game/entities/HotPack.cpp
type: source
path: src/game/entities/HotPack.cpp
domain: game
bucket: entities
loc: 14
classes: []
sources: ["src/game/entities/HotPack.cpp"]
---
# `HotPack.cpp`

> **一句定位**：暖暖包消耗品的效果覆寫——使用時加業力並固定減少雨量 25 點（非歸零）。

## 職責

`HotPack::Consume(Player*)` 是 `ConsumableItem` Template Method 鏈的暖暖包版本。

空 player 守衛後，呼叫 `player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief)` 加業力並以固定量（`kRainRelief`，定義於 `HotPack.h`）減少雨量。說明中明確指出此設計從「歸零」改為「-25 固定減免」，使雨量支柱保有意義——完全歸零會讓暖暖包過於強力。設 `isActive_=false`，發布 `ShowMessage`。

與 `ApplyConsumableEffect("HotPack")` 保持數值一致，由測試共同固定兩條路徑。

## 關鍵內容（類別 / 函式 / 資料）

- `HotPack::Consume(Player*)`：null 守衛 + 業力 + 固定雨量減免 + 失活 + ShowMessage。
- `kKarmaBonus` / `kRainRelief`：定義於 `HotPack.h`（`kRainRelief = 25.0f`）。

## 相依與在架構中的位置

- **#include（往外）**：`HotPack.h`、`Player.h`、`EventBus.h` / `EventSink.h`。
- **被誰使用（往內）**：—（葉節點；消耗品多型 dispatch）。
- **繼承 / 實作 / 體現**：`HotPack` → `ConsumableItem`；覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：Collision 或背包路徑觸發，Sweep 清除。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md) 同於 `EnergyDrink`。設計文件特別指出「雨量支柱保有意義」的平衡決策，說明此覆寫不只是模板填充，而是根據玩法設計主動選擇了特定的數值語義。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/HotPack.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/HotPack.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
