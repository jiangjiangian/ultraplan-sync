---
id: file:src/game/entities/TransparentUmbrella.cpp
type: source
path: src/game/entities/TransparentUmbrella.cpp
domain: game
bucket: entities
loc: 44
classes: []
sources: ["src/game/entities/TransparentUmbrella.cpp"]
---
# `TransparentUmbrella.cpp`

> **一句定位**：傘家族共同基底的 C++ 實作——提供「任務閘」守衛、兩條認領入口（互動/碰撞），以及以 `UmbrellaGlyph` 統一繪製的外觀。

## 職責

`TransparentUmbrella.cpp` 實作傘家族的公共邏輯，所有子類別（`TrueUmbrella`、`FragileUmbrella`、`CursedUmbrella`、`ProfessorTrapUmbrella`）透過繼承共享這三個函式。

**任務閘（`QuestGateOpen`，匿名命名空間）**：若玩家尚未設下 `kFlagPromisedVictim`，則發布 `ShowMessage`（「這把傘不是你的——先去找那位掉了傘的同學問問吧。」）並回傳 `false`，阻止任何傘被提前認領。空 player 指標視為閘開（保留先前的 null 路徑）。

**兩條認領入口**：`Interact(Player*)` 和 `OnPickup(Player*)` 均先通過 `QuestGateOpen` 再呼叫 `BeClaimed(initiator/player)`。這確保無論玩家是主動按 E 互動還是走過自動拾取，都必須先滿足任務前提。

**Render**：呼叫 `nccu::game::gfx::DrawUmbrellaGlyph(renderer, LookForStyle(style_), Rect{position_.x, position_.y, 20.0f, 20.0f})`，以每把傘各自的 `UmbrellaStyle` 選取對應外觀（真傘=藍/破傘=剩手柄/詛咒=暗紫/醜傘=綠/ProfTrap=危險紅）。地面拾取物與結局卡片共用同一份字符，使視覺語義一致。純 `DrawRect`，不含 raylib。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestGateOpen(Player*) -> bool`（匿名命名空間）：任務前置守衛，`kFlagPromisedVictim` 閘。
- `void TransparentUmbrella::Interact(Player*)`：通過閘後呼叫 `BeClaimed`。
- `void TransparentUmbrella::OnPickup(Player*)`：通過閘後呼叫 `BeClaimed`。
- `void TransparentUmbrella::Render(IRenderer&) const`：`DrawUmbrellaGlyph` 統一渲染，`LookForStyle(style_)` 選取外觀。
- `style_`（`UmbrellaStyle`，定義於 `TransparentUmbrella.h`）：傘外觀選擇器。

## 相依與在架構中的位置

- **#include（往外）**：`TransparentUmbrella.h`（`style_` / `BeClaimed` 宣告 / `LookForStyle`）、`EventBus.h` / `EventSink.h`（閘拒絕時發 ShowMessage）、`Player.h`（`HasFlag(kFlagPromisedVictim)`）、`Flags.h`（`kFlagPromisedVictim`）、`IRenderer.h`、`UmbrellaGlyph.h`（`DrawUmbrellaGlyph` / `UmbrellaLook`）。
- **被誰使用（往內）**：所有傘子類別的 .cpp 皆繼承自本基底（`TrueUmbrella` / `FragileUmbrella` / `CursedUmbrella` / `ProfessorTrapUmbrella`）。
- **繼承 / 實作 / 體現**：`TransparentUmbrella` → `Item` → `GameObject`；`BeClaimed` 為純虛函式（[Template Method](../concepts/pat-template.md)），由子類別覆寫。
- **每幀管線 / MVC 角色**：認領在 Collision / 互動階段執行；Render 在 View 階段呼叫。

## OO 概念與設計重點

本檔是 [Template Method](../concepts/pat-template.md) 最清晰的體現：`Interact` / `OnPickup` 定義了「閘控→認領」的骨架流程，`BeClaimed` 是留給子類別填充的「具體步驟」。閘守衛放在公共基底確保任何一條拾取路徑都無法繞過任務前提，這是防止前提條件洩露到各子類別的結構性保護。`UmbrellaGlyph` 共享使外觀成為單一真實來源（DRY），並在 `QuestFlagPickup`（苦主的傘）和結局卡片上重用相同字符。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/TransparentUmbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/TransparentUmbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [CRTP](../concepts/oo-crtp.md) · [Observer](../concepts/pat-observer.md)
