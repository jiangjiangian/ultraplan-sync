---
id: file:include/game/entities/Item.h
type: header
path: include/game/entities/Item.h
domain: game
bucket: entities
loc: 46
classes: [Item]
sources: ["include/game/entities/Item.h"]
---
# `Item.h`

> **一句定位**：所有地圖可拾取道具的最小共同基底，在 `GameObject` 之上添加名稱、可拾取旗標與純虛擬 `OnPickup` 鉤子。

## 職責

`Item` 是道具繼承樹的根抽象類別，在 `GameObject`（位置 + 碰撞盒 + `isActive_`）之上新增三件事：

1. **名稱**（`itemName_`）：道具識別字串，兼作 `Player::consumables_` 背包的 `itemId` 鍵。傘類道具、消耗品、金錢與任務旗標拾取物的名稱各不相同，使背包 UI 能正確分類。

2. **可拾取旗標**（`isPickable_`）：預設為 `true`，允許外部查詢。某些情境（例如任務閘控尚未開啟）可修改此旗標以禁止拾取，而無須從 World 移除物件。

3. **純虛擬 `OnPickup(player)`**：每個具體道具葉類別的拾取效果擴充點。設計意圖是「拾取流程統一入口，效果各異」——雨傘執行 `BeClaimed`、消耗品執行 `Collect`（加入背包）、金錢加 `AddMoney`、任務旗標呼叫 `SetFlag`。

本類別不涉及 raylib，屬於純資料的 Model 層。`GetName()` 與 `IsPickable()` 均為 `[[nodiscard]]` 函式，強制呼叫端不丟棄回傳值。

## 關鍵內容（類別 / 函式 / 資料）

- **`Item(position, hitBox, name)`**：建構子，轉交 `GameObject(position, hitBox)`，初始化 `itemName_`（move）與 `isPickable_(true)`。
- **`virtual void OnPickup(Player*) = 0`**：純虛擬拾取鉤子；葉類別覆寫以施放各自的拾取效果。
- **`const std::string& GetName() const noexcept`**：取道具名稱（`[[nodiscard]]`）。
- **`bool IsPickable() const noexcept`**：查詢是否可拾取（`[[nodiscard]]`）。
- **`std::string itemName_`**（protected）：道具名稱兼背包 itemId。
- **`bool isPickable_`**（protected）：可拾取旗標（預設 true）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/core/GameObject.h`（基底）；`<string>`（名稱儲存）。
- **被誰使用（往內）**：`include/game/entities/CashPickup.h`、`include/game/entities/ConsumableItem.h`、`include/game/entities/QuestFlagPickup.h`、`include/game/entities/TransparentUmbrella.h`（四個直接子類別）。
- **繼承 / 實作 / 體現**：繼承自 `GameObject`；被 `CashPickup`、`ConsumableItem`、`QuestFlagPickup`、`TransparentUmbrella` 繼承。
- **每幀管線 / MVC 角色**：Model 層的道具基底。碰撞系統偵測玩家與 Item 重疊時呼叫 `OnPickup`；幀末 Sweep 依 `isActive_` 移除已消耗的道具。

## OO 概念與設計重點

`Item` 體現了抽象基底的「最小化」原則：它只提供所有道具「必然共有」的東西（名稱、可拾取旗標、拾取鉤子），不假設任何關於效果、價格或種類的細節。純虛擬 `OnPickup` 是 [Template Method](../concepts/pat-template.md) 在更高層次的雛形——拾取「觸發」是統一的，「效果」是多型的。

`isPickable_` 旗標作為 protected 成員而非 private，讓子類別（如 `TransparentUmbrella`）可以根據任務閘控狀態臨時關閉拾取，體現了「狀態」而非「存在性」的可拾取控制（不需移除物件）。整體設計讓 `World` 對道具的持有和遍歷一致化（所有道具都是 `GameObject`），同時保留豐富的行為多型。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/Item.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/Item.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [MVC](../concepts/arch-mvc.md)
