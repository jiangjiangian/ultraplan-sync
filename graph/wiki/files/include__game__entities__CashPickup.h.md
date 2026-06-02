---
id: "file:include/game/entities/CashPickup.h"
type: header
path: include/game/entities/CashPickup.h
domain: game
bucket: entities
loc: 53
classes: [CashPickup]
sources: ["include/game/entities/CashPickup.h"]
---
# `CashPickup.h`

> **一句定位**：一次性地面金錢道具——碰撞時把 `value_` 灌入玩家錢包並失效，以 `WithRoles<CashPickup, Item>` 取得 CRTP mixin 能力，實作 `IDrawable` + `IInteractable` 兩個角色。

## 職責

`CashPickup` 屬 game entities 層，是 game/entities 物件階層中的葉類別，繼承自 `WithRoles<CashPickup, Item>` 並直接實作 `IDrawable` 和 `IInteractable` 兩個 ISP 角色介面。

設計選擇說明：`CashPickup` 直接繼承 `Item`（透過 `WithRoles`）而非走 `ConsumableItem` 中介層，原因是「金錢拾取沒有 `Consume()` 語意可共用」——金錢拾取無動畫、無業力增減、無 `AddConsumable` 步驟，和消耗品的 Template Method 管線不相容，故直接落在 Item 子樹之下。

角色選擇：
- **`IDrawable`**：實作 `Render()` 繪出可見的地面金幣標記（具體字符/圖形由 `.cpp` 決定）。
- **`IInteractable`**：`Interact(Player* initiator)` 直接呼叫 `OnPickup(initiator)`，轉換為拾取效果（把 `value_` 加入玩家錢包並 `isActive_=false`）。
- **無 `IUpdatable`**：金錢不需逐幀更新，故捨棄此角色（不實作）；這也確保 `MovementSystem` 不會 tick 它。

`WithRoles<CashPickup, Item>` 是 CRTP mixin，以 `CashPickup` 自身（葉類別）為鍵，確保編譯期的角色判斷（`HasRole<T>()` 等）正確定位到此型別，無 `dynamic_cast`。

三種面額（5/10/20）由 `GameObjectFactory` 的三個 `ObjectType` 列舉值（`CashPickup5`/`10`/`20`）分別建構，傳入不同的 `value_` 值，`CashPickup` 本身不知道面額分類，只持有 `value_`。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class CashPickup final : public WithRoles<CashPickup, Item>, public IDrawable, public IInteractable` | 葉類別；CRTP mixin + 兩個 ISP 角色。 |
| `CashPickup(Vec2 position, int value)` | 建構子；以世界座標和面額初始化。 |
| `Render(IRenderer&)` | 繪出地面金幣標記；override `IDrawable::Render`。 |
| `Interact(Player* initiator)` | override `IInteractable::Interact`；轉呼叫 `OnPickup(initiator)`。 |
| `OnPickup(Player* player)` | override（`Item` 定義的鉤子）；把 `value_` 加入玩家錢包並 `isActive_=false`。 |
| `Value()` | `[[nodiscard]] int`；讀取面額（診斷/測試用）。 |
| `value_` | `int`；拾取時加入錢包的金額。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/Item.h`（父類別，含 `WithRoles`/`OnPickup`）、`include/engine/math/Vec2.h`（建構子座標型別）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`（工廠建構三種面額）、`src/game/entities/CashPickup.cpp`（方法實作）、`src/game/world/World.cpp`/`WorldSpawn.cpp`（生成/查詢）、`tests/entities/test_cashpickup.cpp`/`test_roles.cpp`、`tests/quest/test_chapter_spawns.cpp`/`test_economy_loop.cpp`。
- **繼承 / 實作 / 體現**：繼承 `include/engine/core/Roles.h`（`WithRoles` mixin）、`include/game/entities/Item.h`；CRTP mixin 體現 [CRTP（oo-crtp）](../concepts/oo-crtp.md) 和 [ISP 角色（oo-isp-roles）](../concepts/oo-isp-roles.md)。
- **每幀管線 / MVC 角色**：遊戲 Model 的葉類別；`IInteractable` 角色使其可在 `DispatchInteract` 階段被 E 鍵觸發；`IDrawable` 角色使其在每幀繪製階段被 View 走訪。無 `IUpdatable`，不參與 `MovementSystem`。

## OO 概念與設計重點

`CashPickup` 是 [CRTP mixin `WithRoles`（oo-crtp）](../concepts/oo-crtp.md) + [ISP 角色（oo-isp-roles）](../concepts/oo-isp-roles.md) 的範例物件：透過 CRTP 以編譯期型別判斷取代 `dynamic_cast`，同時精確選取所需的 ISP 角色介面（`IDrawable`/`IInteractable`），拒絕不需要的角色（`IUpdatable`/`IMortal`），充分體現了介面隔離原則。

`Interact()` 直接委派給 `OnPickup()` 的設計，讓「互動即拾取」的語意在型別系統中清晰表達，測試可直接呼叫 `OnPickup` 驗證金錢加入邏輯，無需模擬整個互動派發流程。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/CashPickup.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/CashPickup.h) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [ISP Roles](../concepts/oo-isp-roles.md)
