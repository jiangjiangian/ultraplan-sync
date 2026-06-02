---
id: file:src/game/entities/Player.cpp
type: source
path: src/game/entities/Player.cpp
domain: game
bucket: entities
loc: 180
classes: []
sources: ["src/game/entities/Player.cpp"]
---
# `Player.cpp`

> **一句定位**：玩家實體的完整實作——WASD 輸入驅動移動、Pipoya 行走動畫、雨量/業力/金錢管理及重生機制。

## 職責

`Player` 繼承自 `WithRoles<Player, Character>`，是遊戲唯一的玩家角色，持有所有關鍵的遊戲狀態：`rainMeter_`（0–100 淋濕程度）、`karma_`（-100–100 道德值）、`hasUmbrella_`（遮蔽旗標）、`money_`（金幣）、詛咒污點計數 `cursedTaint_`，以及背包旗標集（`HasFlag` / `SetFlag` / `ClearFlag`）。

**建構**：速度 180 px/秒（60 FPS 下原本的 3 px/frame，幀率無關）；24×24 碰撞盒；初始 `karma_=50`、`money_=100`。

**輸入與移動（`HandleInput` / `Update`）**：`Update` 呼叫 `HandleInput`（讀 WASD，呼叫 `Character::Move` 積分位移），再推進 Pipoya 動畫。動畫只在 `direction_!=0` 時推進（以 `kFrameDuration=0.15f` 計時）；靜止時 `animStep_=0`（`kWalkColumns[0]=1`，待機欄）。朝向記憶於 `lastFacing_`，使靜止時仍朝最後移動方向。

**Render**：無有效 sprite 顯示藍色方塊。有 sprite 時計算 Pipoya 32×32 圖集欄（`kWalkColumns[animStep_]`）與列（`RowForFacing(lastFacing_)`：主導軸判斷，上=3/下=0/左=1/右=2），繪製 32×32 底部置中於 24×24 碰撞盒，帶 `tint_` 色調（持不同傘可能有視覺染色）。

**業力管理（`AddKarma` / `decreaseKarma` / `ApplyCursedTaintDecay`）**：`AddKarma(delta)` 夾制到 [-100,100] 後發布 `KarmaChanged`（帶符號字串 `%+d`，使 HUD 能顯示「業力 +5」）；delta=0 也發布（無害，訂閱者可過濾）。`ApplyCursedTaintDecay()` 若 `cursedTaint_>0` 則呼叫 `AddKarma(-5 * cursedTaint_)`。

**雨量管理**：`ApplyRain(dt, lethal)`（曝露速率 +5/秒，持傘 no-op，lethal 達上限時 `RespawnAtGate()`）、`DrainRain(dt)`（遮蔽恢復 -10/秒）、`DrainRainBy(amount)`（固定量消耗品減免，夾制到 0）、`ApplyRainSheltered(dt, lethal)`（持傘但仍在戶外 +1.5/秒，約曝露的 30%）。

**重生（`RespawnAtGate`）**：傳送到正門 `{500, 1860}`，歸零雨量，發布 `ShowMessage`（「你淋成落湯雞了，被傳送回正門。半天就這樣過去了。」）。

**金錢（`DeductMoney`）**：防守性，金額超出時回傳 `false` 且無副作用。

## 關鍵內容（類別 / 函式 / 資料）

- `Player(Vec2 position)`：建構，速度 180，碰撞盒 24×24，初始 karma=50 money=100。
- `void Update(float dt)`：輸入 + 動畫推進。
- `void HandleInput(float dt)`：WASD → `Character::Move`。
- `void Render(IRenderer&) const`：Pipoya 圖集渲染，底部置中，帶 `tint_`。
- `void LoadSprite(const string&)`：載入貼圖。
- `Player& AddKarma(int delta)`：夾制 + 發布 `KarmaChanged`，流式。
- `Player& decreaseKarma(int amount)`：轉發 `AddKarma(-amount)`。
- `Player& ApplyCursedTaintDecay()`：`-5 × cursedTaint_` 業力衰減。
- `Player& resetRainMeter() noexcept`：歸零雨量。
- `bool DeductMoney(int amount) noexcept`：防守性扣款。
- `Player& ApplyRain(float dt, bool lethal)`：曝露淋濕（+5/秒，持傘 no-op）。
- `Player& DrainRain(float dt) noexcept`：遮蔽恢復（-10/秒）。
- `Player& DrainRainBy(float amount) noexcept`：固定量消耗品減免。
- `Player& ApplyRainSheltered(float dt, bool lethal)`：持傘戶外淋濕（+1.5/秒）。
- `void RespawnAtGate()`：傳送到 `{500,1860}`，歸零雨量，ShowMessage。
- `rainMeter_` / `karma_` / `hasUmbrella_` / `money_` / `cursedTaint_`：核心遊戲狀態。
- `constexpr int kSpriteCell = 32`、`constexpr float kFrameDuration = 0.15f`：動畫常數。
- `constexpr array<int,4> kWalkColumns = {1,0,1,2}`：Pipoya 待機/左腳/待機/右腳欄序。

## 相依與在架構中的位置

- **#include（往外）**：`Player.h`、`EventBus.h` / `EventSink.h`（KarmaChanged / ShowMessage）、`IRenderer.h` / `Color.h` / `Rect.h`（渲染）、`Input.h` / `Key.h`（WASD 輸入）。
- **被誰使用（往內）**：—（葉節點實作；`World::GetPlayer()` 回傳指標，Controller 與所有 entities 均使用它）。
- **繼承 / 實作 / 體現**：`Player` → `WithRoles<Player, Character>` → `Character` → `GameObject`；實作 `IUpdatable`（`Update`）、`IDrawable`（`Render`）角色。
- **每幀管線 / MVC 角色**：Model 層的核心角色；在 Survival 階段讀取 `rainMeter_`，Movement 階段呼叫 `Update`，View 呼叫 `Render`。

## OO 概念與設計重點

`Player` 是整個架構的狀態中心：業力、雨量、金錢、傘種、背包旗標均在此彙聚。[CRTP mixin](../concepts/oo-crtp.md) 使角色介面靜態分派，`Update`/`Render` 無虛擬函式開銷。[Observer](../concepts/pat-observer.md) 體現於 `KarmaChanged` 事件——業力 HUD 橫幅由訂閱者顯示，`Player` 本身不感知 UI 。`ApplyRainSheltered` 的 +1.5/秒設計（而非持傘完全防雨）是刻意的玩法決策，在行內說明了其緊張感與可通關性之間的平衡。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/Player.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/Player.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [ISP 角色介面](../concepts/oo-isp-roles.md) · [Observer](../concepts/pat-observer.md) · [MVC](../concepts/arch-mvc.md)
