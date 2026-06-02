---
id: "file:include/game/controller/GameObjectFactory.h"
type: header
path: include/game/controller/GameObjectFactory.h
domain: game
bucket: controller
loc: 50
classes: [GameObjectFactory]
sources: ["include/game/controller/GameObjectFactory.h"]
---
# `GameObjectFactory.h`

> **一句定位**：GoF Factory Method——以 `ObjectType` 列舉集中生產所有具體 `GameObject` 子類別，呼叫端只認得抽象基底，新增物件種類只改這一處。

## 職責

`GameObjectFactory.h` 定義了 `ObjectType` 列舉和 `GameObjectFactory` 類別，是整個遊戲物件生成的唯一工廠。屬 game controller 層。

`ObjectType` 列舉列出所有可生產的物件種類：`Player`、傘家族四種（`TrueUmbrella`/`FragileUmbrella`/`ProfessorTrapUmbrella`/`CursedUmbrella`）、消耗品三種（`HotPack`/`WaterproofSpray`/`EnergyDrink`）、`Vendor`（商店 NPC 佔位版）、以及三種面額的金錢拾取物（`CashPickup5`/`CashPickup10`/`CashPickup20`）。每個列舉值對應一個具體 `GameObject` 子類別。

`GameObjectFactory::Create(ObjectType type, Vec2 position)` 是唯一的公開方法，以靜態工廠方式（無需實例化工廠）依種類建構對應子類別，回傳 `unique_ptr<GameObject>`。`type` 無對應時回傳 `nullptr`，讓呼叫端可安全跳過。

這個設計確保「呼叫端只需 `ObjectType` 與座標即可取得抽象 `GameObject`」，不需 `#include` 任何具體實體型別的標頭，符合 OCP（開放/封閉原則）——新增物件種類時只修改 `GameObjectFactory.cpp` 的 switch 和此標頭的 `ObjectType` 列舉，不散落各處改 include 與建構碼。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `enum class ObjectType` | 列出所有可生產物件種類（12 種）。`Vendor` 的注解說明工廠只產佔位版，正式市集攤位由 `main.cpp` 直接建構。`CashPickup5`/`10`/`20` 以「一值一列舉」取代帶參數的設計，提升可讀性。 |
| `class GameObjectFactory` | 靜態工廠，無成員狀態，只有一個靜態方法。 |
| `static Create(ObjectType type, Vec2 position)` → `unique_ptr<GameObject>` | 依種類建構具體子類別；無對應時回傳 `nullptr`。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/core/GameObject.h`（抽象基底，回傳型別）、`include/engine/math/Vec2.h`（位置參數型別）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`（實作）、`src/game/world/World.cpp`（建立初始物件）、`src/game/world/WorldSpawn.cpp`（動態生成物件）；`tests/controller/test_factory.cpp`、`tests/entities/test_cashpickup.cpp`/`test_consumable.cpp`、`tests/vendor/test_vendor.cpp`（測試）。
- **繼承 / 實作 / 體現**：體現 [Factory Method（pat-factory）](../concepts/pat-factory.md)。
- **每幀管線 / MVC 角色**：Controller 層工具；在 `SpawnSystem::Run()` 和 `World` 初始化時呼叫，不直接在管線中執行，而是被管線的 Spawn 階段觸發。

## OO 概念與設計重點

`GameObjectFactory` 是 [Factory Method（pat-factory）](../concepts/pat-factory.md) 的直接實現：以 `ObjectType` 列舉取代類別階層的 Creator/Product 二元分解，把「決定要建構哪個子類別」的知識集中於工廠，呼叫端只面向抽象介面（`GameObject*`）。

靜態方法設計（無需工廠實例）反映了「這裡沒有多型工廠的需求」——整個遊戲只需一種建構策略，靜態方法比繼承更輕。`unique_ptr<GameObject>` 的回傳確保所有權語意明確，不洩漏記憶體。

`CashPickup5`/`10`/`20` 以三個列舉值取代帶參數設計，是一個刻意的可讀性決策（注解中明確說明了這個設計選擇）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/GameObjectFactory.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameObjectFactory.h) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md)
