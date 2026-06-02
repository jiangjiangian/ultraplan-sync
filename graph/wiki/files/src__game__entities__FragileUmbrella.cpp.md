---
id: file:src/game/entities/FragileUmbrella.cpp
type: source
path: src/game/entities/FragileUmbrella.cpp
domain: game
bucket: entities
loc: 14
classes: []
sources: ["src/game/entities/FragileUmbrella.cpp"]
---
# `FragileUmbrella.cpp`

> **一句定位**：脆弱傘的認領覆寫——記錄持有種類、失活，並告知玩家骨架已斷裂、雨水仍會滲入。

## 職責

`FragileUmbrella::BeClaimed(Player*)` 是傘家族 Template Method 鏈的脆弱傘版本，邏輯極為精簡。

空 player 守衛 + `!isActive_` 冪等守衛後，呼叫 `player->SetHeldUmbrella(HeldUmbrella::Fragile)` 同時記錄背包欄位與開啟遮蔽。設 `isActive_=false`，發布 `UmbrellaClaimed("FragileUmbrella")`（觸發接線中的章節清關邏輯）以及 `ShowMessage`（告知骨架斷裂、雨仍會慢慢滲）。

`FragileUmbrella` 沒有任何業力效果也沒有設旗標——它是一把功能性但不完美的傘（`ApplyRainSheltered` 只緩慢淋濕，而非完全防雨）。

## 關鍵內容（類別 / 函式 / 資料）

- `FragileUmbrella::BeClaimed(Player*)`：null + 冪等守衛 + `SetHeldUmbrella(Fragile)` + 失活 + 兩則事件。
- `Player::SetHeldUmbrella(HeldUmbrella::Fragile)`：記錄背包欄位並設 `hasUmbrella_=true`。

## 相依與在架構中的位置

- **#include（往外）**：`FragileUmbrella.h`、`Player.h`、`EventBus.h` / `EventSink.h`。
- **被誰使用（往內）**：—（葉節點；`TransparentUmbrella::BeClaimed` 的具體覆寫終端）。
- **繼承 / 實作 / 體現**：`FragileUmbrella` → `TransparentUmbrella` → `Item`；`BeClaimed` 覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：認領時在 Collision / 互動階段執行，Sweep 清除。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md)：和 `CursedUmbrella` / `TrueUmbrella` / `ProfessorTrapUmbrella` 共享認領骨架，各自只覆寫「具體效果」的那一步。`FragileUmbrella` 是傘家族中副作用最小的版本，體現出 Template Method 允許在不改變流程骨架的前提下靈活調整子步驟。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/FragileUmbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/FragileUmbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
