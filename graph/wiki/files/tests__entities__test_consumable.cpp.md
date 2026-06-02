---
id: "file:tests/entities/test_consumable.cpp"
type: test
path: tests/entities/test_consumable.cpp
domain: tests
bucket: entities
loc: 263
classes: [MessageCapture, Row]
sources: ["tests/entities/test_consumable.cpp"]
---
# `test_consumable.cpp`

> **一句定位**：驗證可消耗道具的三層語意——直接 `Consume()` 的 karma 與雨量計效果、`OnPickup/Interact` 只放進背包不立即生效、從背包使用的 `ApplyConsumableEffect` 效果與 `IsUsableConsumable` 分類，以及 `DrainRainBy` 的地板裁切。

## 職責

本檔包含 11 個 `TEST_CASE`，系統性地覆蓋 `ConsumableItem` 三個子類別（HotPack/WaterproofSpray/EnergyDrink）及背包使用路徑。

**`MessageCapture` fixture**：使用 `ScopedSubscribe`（而非裸 Subscribe），確保 handler 生命週期綁在 fixture 的 scope，避免跨 case 的懸空捕捉 UAF。

**直接 Consume 效果**：
- `HotPack`：先灌 60 雨量，`Consume` 後 karma `+HotPack::kKarmaBonus`（+5）、雨量 `-HotPack::kRainRelief`（-25）→ 35、停用、ShowMessage 正確文字。
- `WaterproofSpray`：karma 不動（裝備類）、雨量 `-kRainRelief`（-35）→ 45、停用、ShowMessage 正確。
- `EnergyDrink`：karma +3、雨量 -15→ 25、停用、ShowMessage 正確。
- null 玩家安全：`Consume(nullptr)` 後道具維持啟用、不發事件。

**Factory 動態型別**：三個案例各以 `GameObjectFactory::Create` 建立對應型別，`dynamic_cast` 驗證。

**撿取只放入背包**：`HotPack::OnPickup(&p)` 後 `ConsumableCount("HotPack") == 1`、karma 不動、雨量不動、無事件；物件停用。冪等：再撿一次背包不變。`EnergyDrink::Interact(&p)` 同路徑（背包 +1，不立即套用效果）。

**從背包使用**：`ApplyConsumableEffect(EventBus, player, "HotPack")` 套用與 `Consume()` 完全一致的效果（karma +5、雨量 -25、ShowMessage 同文字）；呼叫方以 `ConsumeOne("HotPack")` 扣減計數。`EnergyDrink` 同驗。

**雨量緩解對照表**（`struct Row`）：7 個道具的預期 relief 與 `IsUsableConsumable` 結果：WaterproofSpray 35、HotPack 25、EnergyDrink 15、EggCake/FlowerTea/Takoyaki 各 15、Donation 0（非可使用）。逐項以 `ApplyConsumableEffect` 驗證 90 雨量後的扣減值，並驗證地板裁切（低雨量時不低於 0）。`DrainRainBy` 單元：50 - 20 = 30；50 - 100 = 0（裁切）。

**`IsUsableConsumable` 分類**：HotPack/EnergyDrink/WaterproofSpray 為 true；`kItemMoney`/`kItemTrueUmbrella`/`"NotAThing"` 為 false。

## 關鍵內容（類別 / 函式 / 資料）

- `struct MessageCapture`：ScopedSubscribe ShowMessage 捕捉器。
- `struct Row`：雨量緩解對照表的一列（`id`、`relief`、`usable`）。
- `HotPack::kKarmaBonus`（+5）、`HotPack::kRainRelief`（25）、`HotPack::GetPrice()`（30）。
- `WaterproofSpray::kRainRelief`（35）、`GetPrice()`（50）。
- `EnergyDrink::kKarmaBonus`（+3）、`EnergyDrink::kRainRelief`（15）、`GetPrice()`（40）。
- `nccu::ApplyConsumableEffect(EventBus, Player, itemId)`：從背包使用的效果套用。
- `nccu::IsUsableConsumable(itemId)`：可使用消耗品分類查詢。
- `Player::AddConsumable`、`ConsumableCount`、`ConsumeOne`、`DrainRainBy`。

## 相依與在架構中的位置
- **#include（往外）**：`ConsumableItem.h`、`HotPack.h`、`WaterproofSpray.h`、`EnergyDrink.h`、`GameObjectFactory.h`、`ItemCatalog.h`、`Player.h`、`EventBus.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

本檔完整釘住了 [Template Method](../concepts/pat-template.md) `Consume` 在三個具體子類別的分化效果，以及「撿取放背包 / 從背包使用」的兩階段語意。`MessageCapture` 使用 `ScopedSubscribe` 解決了之前裸 Subscribe 的懸空捕捉問題，是 [RAII](../concepts/oo-raii.md) 在測試安全性上的直接應用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_consumable.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_consumable.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-template](../concepts/pat-template.md) · [RAII](../concepts/oo-raii.md)
