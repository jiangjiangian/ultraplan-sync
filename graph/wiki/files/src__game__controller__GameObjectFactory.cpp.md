---
id: "file:src/game/controller/GameObjectFactory.cpp"
type: source
path: src/game/controller/GameObjectFactory.cpp
domain: game
bucket: controller
loc: 35
classes: []
sources: ["src/game/controller/GameObjectFactory.cpp"]
---
# `GameObjectFactory.cpp`

> **一句定位**：Factory Method 的具體實作：依 `ObjectType` 列舉分派，以 `make_unique` 建立所有遊戲物件型別。

## 職責

`GameObjectFactory::Create(ObjectType type, Vec2 position)` 是一個純 switch 分派函式，依列舉值建立對應的 `unique_ptr<GameObject>`：

- 角色：`Player`
- 傘家族：`TrueUmbrella`、`FragileUmbrella`、`ProfessorTrapUmbrella`、`CursedUmbrella`
- 消耗品：`HotPack`、`WaterproofSpray`、`EnergyDrink`
- 商販：`Vendor`（佔位攤位，帶預設庫存 `VendorConfig{"市集攤主", "歡迎光臨", {{"HotPack", 30}}}`）
- 硬幣拾取：`CashPickup(position, 5/10/20)`（三種面額）

`Vendor` 分支有一條佔位注解：正式市集攤位於 `main.cpp` 建好含完整庫存清單的 `VendorConfig` 後直接傳入，此處的預設僅作為後備。所有未匹配的 ObjectType 回傳 `nullptr`（switch 後的 fallthrough 守護）。

## 關鍵內容（類別 / 函式 / 資料）

- `GameObjectFactory::Create(ObjectType type, Vec2 position) -> unique_ptr<GameObject>` — 全 ObjectType 列舉的工廠分派；各型別以 `make_unique` 建構。
- `ObjectType` 覆蓋：`Player`、`TrueUmbrella`、`FragileUmbrella`、`ProfessorTrapUmbrella`、`CursedUmbrella`、`HotPack`、`WaterproofSpray`、`EnergyDrink`、`Vendor`、`CashPickup5/10/20`（共 12 種）。

## 相依與在架構中的位置
- **#include（往外）**：`GameObjectFactory.h`；所有實體標頭（`CashPickup.h`、`CursedUmbrella.h`、`EnergyDrink.h`、`FragileUmbrella.h`、`HotPack.h`、`Player.h`、`ProfessorTrapUmbrella.h`、`TrueUmbrella.h`、`WaterproofSpray.h`）；`Vendor.h`、`VendorConfig.h`
- **被誰使用（往內）**：—（由 World 初始化或 NpcSpawns / SpawnSystem 呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；物件生成（SpawnSystem 或初始化）；不在每幀固定管線中，只在需要建立物件時呼叫

## OO 概念與設計重點

此檔案是 [Factory Method](../concepts/pat-factory.md) 的具體體現：`ObjectType` 列舉是抽象化的「物件種類」，`Create` 是工廠方法，呼叫端不需 `#include` 各實體標頭，只需 `#include GameObjectFactory.h`。所有物件以 `unique_ptr<GameObject>` 回傳，確保所有權明確（[RAII](../concepts/oo-raii.md)）。`Vendor` 的佔位預設設計使工廠在測試場景下也可正常建立攤販物件，不需正式的 `VendorConfig`。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/GameObjectFactory.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/GameObjectFactory.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Factory](../concepts/pat-factory.md) · [RAII](../concepts/oo-raii.md)
