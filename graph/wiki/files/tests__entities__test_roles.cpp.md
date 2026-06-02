---
id: file:tests/entities/test_roles.cpp
type: test
path: tests/entities/test_roles.cpp
domain: tests
bucket: entities
loc: 204
classes: [Bare]
sources: ["tests/entities/test_roles.cpp"]
---
# `test_roles.cpp`

> **一句定位**：驗證以 ISP 切出的角色介面（IUpdatable / IDrawable / IInteractable / IMortal）透過 CRTP `WithRoles` 靜態分派的正確性——每個實體透過 `GameObject&` 的 `As*()` 存取器均能精確解析，且 `ForEachRole` 的走訪只觸及扮演對應角色的物件。

## 職責

此測試是 `Roles.h` 系統的規格文件，以九個 TEST_CASE 覆蓋整個角色模型。所有查詢都透過 `GameObject&`（正是 World 容器看待物件的方式），藉此證明靜態的 `As*()` 存取器能透過執行期多型的基底正確解析，且全程不使用 `dynamic_cast`。

測試確立以下不變式：
1. 「已捨棄的空操作角色」必須回傳 `nullptr`（不扮演的角色就不扮演）。
2. `ForEachRole<R>` 只走訪實際扮演 R 的活躍物件，失活物件被跳過。
3. 回傳的指標確實能分派到具體覆寫（不只是非 null，能真正觸發 `OnPickup`）。
4. 碰撞層位預設 0 且可設定。

## 關鍵內容（類別 / 函式 / 資料）

- `Bare`（局部 struct）：最小的 `GameObject` 子類，不混入任何 `WithRoles`；用於驗證三個存取器均回傳 `nullptr`。
- `TEST_CASE("Player 扮演 Update + Draw，但不扮演 Interact")`：`AsInteractable() == nullptr` 確認舊的空操作已捨棄。
- `TEST_CASE("NPC 扮演全部三種角色")`：三個存取器全非 null。
- `TEST_CASE("Vendor（NPC 子類）透過 WithRoles 繼承 NPC 的完整角色集")`：加上 `IsVendor()` 查詢。
- `TEST_CASE("ConsumableItem 只扮演 Interact")`：用 `HotPack` 和 `EnergyDrink` 驗證 Update/Render 均 null。
- `TEST_CASE("雨傘扮演 Draw + Interact，但不扮演 Update")`：`TrueUmbrella` 的 Update 為 null。
- `TEST_CASE("金錢／任務拾取物扮演 Draw + Interact，但不扮演 Update")`：`CashPickup` 和 `QuestFlagPickup`。
- `TEST_CASE("靜態存取器回傳的指標確實能分派到具體覆寫")`：透過 `g.AsInteractable()->Interact(&p)` 觸發 `QuestFlagPickup` 的 OnPickup，斷言旗標被設且物件停用。
- `TEST_CASE("ForEachRole<IUpdatable> 只走訪會逐幀更新的物件")`：混合容器 5 個物件中只有 Player + NPC 扮演 IUpdatable（visited == 2）；前者 Deactivate 後 visited == 1。
- `TEST_CASE("Player 扮演 IMortal 角色；NPC／道具不扮演")`：`AsMortal()` 的正/反向驗證。
- `TEST_CASE("IMortal：TakeDamage 降低 hp、裁切於 0、IsDead 翻轉")`：含負值忽略與超量裁切。
- `TEST_CASE("ForEachRole<IMortal> 只走訪有生命值的實體並分派傷害")`：visited == 1，傷害確實生效。
- `TEST_CASE("GameObject 碰撞層位：預設為 0，可設定")`：`GetCollisionLayer` / `SetCollisionLayer`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/core/GameObject.h`，`engine/core/Roles.h`（`WithRoles`、`ForEachRole`），`engine/events/EventBus.h`，以及 `Player`、`NPC`、`HotPack`、`EnergyDrink`、`TrueUmbrella`、`CashPickup`、`QuestFlagPickup`、`Vendor`、`VendorConfig`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Bare` 繼承 `GameObject`。
- **每幀管線 / MVC 角色**：—（純邏輯單元測試）

## OO 概念與設計重點

本測試是 [ISP Roles](../concepts/oo-isp-roles.md) 設計的核心規格：[CRTP mixin](../concepts/oo-crtp.md) `WithRoles<Derived,Base>` 使得「扮演哪些角色」成為編譯期決策，而本測試從執行期驗證其語意的正確性——所有斷言都透過 `GameObject&` 基底，模擬 World 場景容器的視角，同時完全不使用 `dynamic_cast`。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_roles.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_roles.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[ISP Roles](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
