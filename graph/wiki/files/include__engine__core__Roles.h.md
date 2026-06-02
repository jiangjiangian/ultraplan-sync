---
id: file:include/engine/core/Roles.h
type: header
path: include/engine/core/Roles.h
domain: engine
bucket: core
loc: 151
classes: [IUpdatable, IDrawable, IInteractable, IMortal, WithRoles]
sources: ["include/engine/core/Roles.h"]
---
# `Roles.h`

> **一句定位**：角色介面拆分（ISP）與 CRTP 靜態分派 mixin 的核心定義，讓實體只宣告自己真正扮演的能力，消除 dynamic_cast，並提供泛型 `ForEachRole` 遍歷輔助。

## 職責

此標頭是整個物件模型「能力系統」的根源，解決「肥大基底介面」問題。在重構前，`GameObject` 強制每個子類別都實作 `Update / Render / Interact`，但大量子類別（如 `ConsumableItem` 的 `Update`，`Player` 的 `Interact`）只能提供空殼 no-op。

**四個角色介面**將能力明確拆分：
- `IUpdatable`：`Update(float deltaTime)` — 每幀推進自身狀態。
- `IDrawable`：`Render(IRenderer&) const` — 透過抽象渲染器繪製。
- `IInteractable`：`Interact(Player*)` — 被玩家以 E 鍵觸發。
- `IMortal`：`TakeDamage(int) noexcept`、`IsDead() const noexcept`、`Hp() const noexcept` — 可受傷 / 可被擊殺（目前僅 Player 扮演）。

**CRTP mixin `WithRoles<Derived, Base>`** 在編譯期以 `std::derived_from<Derived, Role> + if constexpr` 偵測 `Derived` 繼承了哪些角色介面，據此在 `AsUpdatable / AsDrawable / AsInteractable / AsMortal` 中回傳 `static_cast<Derived*>(this)` 或 `nullptr`——無 `dynamic_cast`，無執行期型別查詢。它插入於既有繼承層次的中間層（如 `ConsumableItem / TransparentUmbrella / NPC`），使整個角色集合在中介層確定後，下層所有葉類別自動繼承。

**`ForEachRole<Role, Container, F>`** 是泛型遍歷輔助：對容器中扮演指定 `Role`（`IUpdatable / IInteractable / IMortal`）的每個存活物件呼叫 `fn`，以 `if constexpr` 在編譯期解析 `As<Role>()` 的對映，避免每個呼叫點都要重寫遍歷邏輯。`IDrawable`（`const`）的 Render 路徑由 `View` 直接分派（需要畫家演算法排序），故刻意不走此輔助。

## 關鍵內容（類別 / 函式 / 資料）

- **`IUpdatable`**：可更新角色介面。`Update(float deltaTime) = 0`。
- **`IDrawable`**：可繪製角色介面。`Render(IRenderer& renderer) const = 0`。
- **`IInteractable`**：可互動角色介面。`Interact(Player* initiator) = 0`。
- **`IMortal`**：可受傷角色介面。`TakeDamage(int amount) noexcept = 0`（noexcept，戰鬥熱迴圈）、`IsDead() const noexcept = 0`、`Hp() const noexcept = 0`。
- **`WithRoles<Derived, Base>`**（CRTP class template）：靜態分派 mixin，以 `std::derived_from + if constexpr` 實作四個能力查詢（`AsUpdatable / AsDrawable / AsInteractable / AsMortal`）。`using Base::Base` 繼承 Base 的所有建構子。
- **`ForEachRole<Role, Container, F>(Container&, F&&)`**（自由函式模板）：泛型角色遍歷輔助，跳過非存活或不扮演指定 Role 的物件；`if constexpr` 編譯期選路，zero overhead。

## 相依與在架構中的位置

- **#include（往外）**：僅 `<concepts>`（C++20 `std::derived_from`）——不依賴任何引擎或遊戲標頭，保持最底層的可複用性；以前向宣告引入 `IRenderer` 和 `Player`（兩個角色介面函式簽章需要）。
- **被誰使用（往內）**：`include/engine/core/GameObject.h`（宣告 `As*` 純虛擬）；多個實體標頭繼承（`CashPickup / ConsumableItem / DlcSign / NPC / Player / QuestFlagPickup / TransparentUmbrella`）；`src/game/controller/SimSystems.cpp`（`ForEachRole` 遍歷）；`tests/entities/test_roles.cpp`（直接測試）。
- **繼承 / 實作 / 體現**：被上述實體類別繼承；`WithRoles` 插入於繼承層次中間。
- **每幀管線 / MVC 角色**：engine/core 層的能力系統；每幀 Survival / Movement / Collision 等系統透過 `ForEachRole<IUpdatable>` 批次遍歷可更新物件。

## OO 概念與設計重點

此檔是 [ISP（介面隔離原則）](../concepts/oo-isp-roles.md) 最直接的體現：把肥大的單一基底介面拆成四個細粒度角色介面，讓實體只繼承自己真正需要的能力。

[CRTP 靜態 mixin](../concepts/oo-crtp.md) 的 `WithRoles` 是「在編譯期實作執行期多型查詢」的範例：`if constexpr + std::derived_from` 讓能力判斷在零執行期成本下完成，`static_cast` 保證型別正確性（比 `dynamic_cast` 更快且更安全——只在 `Derived` 確實繼承時才轉型）。

`ForEachRole` 是策略模式的泛型化：呼叫端傳入 `fn` 定義「對該 Role 做什麼」，`ForEachRole` 定義「怎麼找到所有扮演該 Role 的物件」，兩者分離，符合 [Strategy 模式](../concepts/pat-strategy.md) 精神。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/core/Roles.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/Roles.h) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [ISP / 角色介面](../concepts/oo-isp-roles.md) · [Strategy](../concepts/pat-strategy.md)
