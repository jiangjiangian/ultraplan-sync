---
id: file:include/game/entities/EnergyDrink.h
type: header
path: include/game/entities/EnergyDrink.h
domain: game
bucket: entities
loc: 34
classes: [EnergyDrink]
sources: ["include/game/entities/EnergyDrink.h"]
---
# `EnergyDrink.h`

> **一句定位**：消耗品葉類別，使用時加 3 業力並減 15 點雨量，並在 Ch2 中充當喚醒學霸的必要道具。

## 職責

`EnergyDrink` 是 `ConsumableItem` 的三個具體葉類別之一，象徵考前士氣提振。它覆寫 `Consume(player)` 施放效果：加 `kKarmaBonus`（3）業力並以 `DrainRainBy(kRainRelief)` 減 15 點雨量，單次使用後即失效。

本類別宣告了三個 `static constexpr` 常數作為單一事實來源：`kPrice = 40`（攤販售價），`kKarmaBonus = 3`（業力加成），`kRainRelief = 15.0f`（雨量減免）。以常數集中管理使 Vendor 定價、購買提示與效果計算永不漂移。

重要的跨章節角色：`TryRescueBookworm`（Ch2）在喚醒學霸時消耗一瓶 EnergyDrink，使它不只是普通消耗品，而是任務鏈中的必要媒介物。攤販以 35 元販賣（圖書館地下室自販機），而地圖散落的零錢（Ch2 共計 40 元）確保身無分文的玩家仍能湊齊。

## 關鍵內容（類別 / 函式 / 資料）

- **`static constexpr int kPrice = 40`**：攤販售價。
- **`static constexpr int kKarmaBonus = 3`**：使用時的業力加成。
- **`static constexpr float kRainRelief = 15.0f`**：使用時減少的雨量點數。
- **`EnergyDrink(position)`**：建構子，名稱 `"EnergyDrink"`，售價 `kPrice`，轉交 `ConsumableItem`。
- **`void Consume(Player* player) override`**：施放效果，定義於 `.cpp`。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/ConsumableItem.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`（工廠生成）、`src/game/entities/EnergyDrink.cpp`（實作）、`src/game/quest/ItemCatalog.cpp`（道具目錄）、`tests/entities/test_consumable.cpp`、`tests/entities/test_roles.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `ConsumableItem`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層道具；地圖上由 `WorldSpawn` 生成，也可由攤販購買（`Vendor::TryBuy` 呼叫 `AddConsumable("EnergyDrink")`）；`GameController::ApplyConsumableEffect` 消費時呼叫 `Consume()`。

## OO 概念與設計重點

`EnergyDrink` 是最輕量的葉類別實作：只宣告三個常數與建構子，`Consume()` 的主體在 `.cpp` 中。`static constexpr` 常數的使用符合「單一事實來源」原則，使攤販定價（`kNpcCh2Vendor` 設定）與效果數值永遠一致。

作為 [Template Method](../concepts/pat-template.md) 的葉類別，它只需關心「效果是什麼」，無須知道拾取流程（由 `ConsumableItem::Collect` 定稿）。Ch2 任務鏈的「必要媒介」設計體現了道具如何跨越純資料邊界、成為敘事推進器。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/EnergyDrink.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/EnergyDrink.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
