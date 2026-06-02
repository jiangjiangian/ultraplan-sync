---
id: file:src/game/entities/CashPickup.cpp
type: source
path: src/game/entities/CashPickup.cpp
domain: game
bucket: entities
loc: 42
classes: []
sources: ["src/game/entities/CashPickup.cpp"]
---
# `CashPickup.cpp`

> **一句定位**：地面金幣拾取物的實作——繪製金色小硬幣、拾取時給玩家加錢並發布 ShowMessage。

## 職責

`CashPickup` 繼承自 `WithRoles<CashPickup, Item>`，是世界中可拾起的金錢道具，`value_` 決定拾取所得金額。

**建構**：接受 `Vec2 position` 與 `int value`，以 16×16 矩形作為碰撞盒（與 `ConsumableItem` 相同尺寸，走相同的碰撞拾取掃描路徑），類型字串為 `"Cash"`。

**Render**：在碰撞盒中央繪製一個 10×10 的金色（`Colors::Gold`）實心小方塊（置中計算用 `inset = (hitBox_.width - kCoin) * 0.5f`）。遵守 `Item` 的架構規則：不呼叫 `DrawText` / `DrawTexture`，不包含 raylib。金色是「可撿取金錢」的普世視覺語義，比舊版全色碰撞盒更易辨識。

**OnPickup**：空 player 指標視為無動作（防守性 null-check）。命中時呼叫 `player->AddMoney(value_)`，設 `isActive_ = false`（標記待幀末 Sweep），並透過 `EventSink().Publish` 發布 `ShowMessage`（「撿到 N 元」）。

## 關鍵內容（類別 / 函式 / 資料）

- `CashPickup(Vec2 position, int value)`：建構，16×16 碰撞盒，類型字串 `"Cash"`。
- `void Render(IRenderer&) const`：金色 10×10 中心小方塊，不含 raylib。
- `void OnPickup(Player*)`：加錢 + 失活 + ShowMessage；null player 為 no-op。
- `value_`（int）：拾取所得金額。
- `constexpr float kCoin = 10.0f`：硬幣圖形尺寸（局部常數）。

## 相依與在架構中的位置

- **#include（往外）**：`CashPickup.h`、`EventBus.h` / `EventSink.h`（發布 ShowMessage）、`Player.h`（`AddMoney`）、`Color.h` / `Rect.h` / `IRenderer.h`（渲染）。
- **被誰使用（往內）**：—（葉節點；由 `GameObjectFactory` 或 `World::SpawnChapterNpcs` 建立實例）。
- **繼承 / 實作 / 體現**：繼承 `WithRoles<CashPickup, Item>` → `Item` → `GameObject`；實作 `IDrawable`（`Render`）、`IInteractable`（`OnPickup`）角色介面。
- **每幀管線 / MVC 角色**：Model 層實體；在 Collision 階段被拾取（`OnPickup`），在 Sweep 階段被清除。

## OO 概念與設計重點

`CashPickup` 是 [CRTP mixin `WithRoles`](../concepts/oo-crtp.md) 的典型使用者，以靜態多型在編譯期掛載角色，無 `dynamic_cast`。[Observer 模式](../concepts/pat-observer.md) 體現於 `EventSink().Publish(ShowMessage)`——`CashPickup` 發布事件，由 HUD 訂閱者顯示拾取提示，兩者無直接相依。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/CashPickup.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/CashPickup.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[CRTP](../concepts/oo-crtp.md) · [Observer](../concepts/pat-observer.md)
