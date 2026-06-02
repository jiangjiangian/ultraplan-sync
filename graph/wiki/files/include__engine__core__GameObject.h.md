---
id: file:include/engine/core/GameObject.h
type: header
path: include/engine/core/GameObject.h
domain: engine
bucket: core
loc: 145
classes: [GameObject]
sources: ["include/engine/core/GameObject.h"]
---
# `GameObject.h`

> **一句定位**：地圖上所有「東西」（玩家、NPC、道具、裝飾）的抽象基底，持有最小共同狀態並以角色介面（ISP）取代肥大純虛擬，支援無 `dynamic_cast` 的能力查詢。

## 職責

`GameObject` 是整個物件模型的根基，但刻意保持「最小共同狀態」原則：只持有 `position_`（世界座標）、`hitBox_`（AABB 碰撞盒）、`isActive_`（存活旗標）和 `collisionLayer_`（碰撞層），不含任何渲染或輸入邏輯。

能力查詢（`AsUpdatable / AsDrawable / AsInteractable / AsMortal`）均回傳指標而非強制每個子類別實作——回傳 `nullptr` 表示「不扮演此角色」。子類別透過 CRTP mixin `WithRoles<Derived, Base>`（定義於 `Roles.h`）在編譯期實作這些查詢，完全不用 `dynamic_cast`。

`isActive_` 旗標實作 mark-then-sweep 的延後刪除：`Deactivate()` 設為 `false`，幀末的 `World::Sweep()` 再真正移除，避免在迭代途中刪除物件。

此外 `GameObject` 提供幾個「以虛擬函式取代 dynamic_cast」的查詢旗標：`BlocksMovement()`（預設 `false`，NPC / 牆覆寫為 `true`）、`DialogLines()`（可對話者回傳台詞指標）、`NpcId()`（對話查找識別字串）、`IsVendor()`（商店攤販旗標）、`IsQuestGiver()`（任務給予者旗標）。每個旗標都解決了一個具體問題，例如 `IsVendor()` 防止 E 互動誤把攤販導向 NPC 台詞循環。

## 關鍵內容（類別 / 函式 / 資料）

- **`GameObject`**：所有遊戲物件的抽象基底。
  - `GameObject(Vec2 position, Rect hitBox)`：建構，預設存活、碰撞層 0。
  - `AsUpdatable() noexcept → IUpdatable*`：能力查詢；`WithRoles` mixin 在編譯期偵測並回傳 `static_cast` 指標，或 `nullptr`。
  - `AsDrawable() const noexcept → const IDrawable*`：可繪製能力查詢。
  - `AsInteractable() noexcept → IInteractable*`：可互動能力查詢。
  - `AsMortal() noexcept → IMortal*`：可受傷能力查詢（目前僅 Player 扮演）。
  - `CheckCollision(Rect other) const noexcept → bool`：AABB 碰撞測試，委託 `hitBox_.Intersects(other)`。
  - `IsActive() const noexcept → bool`：存活旗標查詢。
  - `Deactivate() noexcept`：標記為待移除（mark-then-sweep 的 mark 階段）。
  - `GetPosition() const noexcept → Vec2`：取得世界座標。
  - `GetCollisionLayer() const noexcept → int`：取得碰撞層（0 = 預設層）。
  - `SetCollisionLayer(int) noexcept`：設定碰撞層。
  - `BlocksMovement() const noexcept → bool`：是否阻擋移動（預設 `false`）。
  - `DialogLines() const noexcept → const vector<string>*`：可對話台詞指標（預設 `nullptr`）。
  - `NpcId() const noexcept → string_view`：NPC 識別字串（預設空）。
  - `IsVendor() const noexcept → bool`：是否為攤販（預設 `false`）。
  - `IsQuestGiver() const noexcept → bool`：是否為任務給予者（預設 `false`）。
- **保護資料成員**：`position_`（Vec2）、`hitBox_`（Rect）、`isActive_`（bool）、`collisionLayer_`（int）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`、`include/engine/math/Rect.h`（幾何基元）、`include/engine/core/Roles.h`（角色介面 + WithRoles mixin）；以前向宣告引入 `IRenderer` 和 `Player`（角色鉤子需要 Player* 但不需完整型別）。
- **被誰使用（往內）**：`Character.h`、`Item.h`、`DlcSign.h`（直接子類別）、`GameObjectFactory.h`、`GameObjectQueries.h`、`World.h`，以及大量測試檔。
- **繼承 / 實作 / 體現**：被 `Character`、`Item`、`DlcSign` 繼承；`Player`、`NPC` 在 `Character` 下；傘家族和消耗品在 `Item` / `Character` 下。
- **每幀管線 / MVC 角色**：engine/core 層的 Model 基底；`World` 持有 `vector<unique_ptr<GameObject>>`，每幀管線的 Survival / Movement / Collision 都以 `AsUpdatable / AsInteractable` 遍歷此容器。

## OO 概念與設計重點

`GameObject` 結合了 [ISP（介面隔離原則）](../concepts/oo-isp-roles.md) 和 [CRTP 靜態多型](../concepts/oo-crtp.md)。四個能力查詢（`As*`）取代了傳統的肥大純虛擬基底，讓「不可更新的道具」、「不可互動的裝飾」不需實作空殼函式。`WithRoles` CRTP mixin 在編譯期以 `std::derived_from + if constexpr` 靜態判斷能力，完全避免 `dynamic_cast` 的執行期成本與不安全性。

`Deactivate / IsActive` 實作 mark-then-sweep，防止迭代途中刪除物件造成迭代器失效。虛擬旗標函式（`BlocksMovement / IsVendor / IsQuestGiver`）以虛擬分派取代 `dynamic_cast`——在繼承封閉的情境下比 `dynamic_cast` 更高效且更安全。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/core/GameObject.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/GameObject.h) · [← 全檔索引](../files-index.md) · 相關概念：[ISP / 角色介面](../concepts/oo-isp-roles.md) · [CRTP](../concepts/oo-crtp.md)
