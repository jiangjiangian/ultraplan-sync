---
id: file:src/game/entities/ProfessorTrapUmbrella.cpp
type: source
path: src/game/entities/ProfessorTrapUmbrella.cpp
domain: game
bucket: entities
loc: 20
classes: []
sources: ["src/game/entities/ProfessorTrapUmbrella.cpp"]
---
# `ProfessorTrapUmbrella.cpp`

> **一句定位**：教授陷阱傘的認領覆寫——設下跨章節負面漣漪旗標，並模擬生成追逐玩家的助教 NPC。

## 職責

`ProfessorTrapUmbrella::BeClaimed(Player*)` 是傘家族中副作用最嚴重的版本，是 Ch1 偷拿教授傘的因果起點。

冪等守衛後，呼叫 `player->SetHeldUmbrella(HeldUmbrella::ProfessorTrap)` 記錄背包欄位並開啟遮蔽。立即設下 `kFlagHasProfessorTrap`——這是 Ch2/Ch3/Ch4 整條負面漣漪鏈的源旗標（助教 -10 / Ch3 -10 / Ch4 對峙 -15）；若此旗標不存在，整條鏈無法觸發。

設 `spawnedEnemiesCount_ = 3` 模擬生成三個追逐助教 NPC（實際生成由 GameWorld 在接收 `UmbrellaClaimed` 事件後執行）。設 `isActive_=false`，發布 `UmbrellaClaimed("ProfessorTrapUmbrella")` 和 `ShowMessage`（「遠處傳來助教們的腳步聲！」）。

## 關鍵內容（類別 / 函式 / 資料）

- `ProfessorTrapUmbrella::BeClaimed(Player*)`：冪等守衛 + `SetHeldUmbrella(ProfessorTrap)` + `SetFlag(kFlagHasProfessorTrap)` + 設模擬計數 + 失活 + 兩則事件。
- `kFlagHasProfessorTrap`（引自 `Flags.h`）：跨 Ch2/Ch3/Ch4 的漣漪因旗標。
- `spawnedEnemiesCount_`：模擬生成助教數量（=3）。

## 相依與在架構中的位置

- **#include（往外）**：`ProfessorTrapUmbrella.h`、`Player.h`、`EventBus.h` / `EventSink.h`、`Flags.h`（`kFlagHasProfessorTrap`）。
- **被誰使用（往內）**：—（葉節點；`TransparentUmbrella::BeClaimed` 的具體終端）。
- **繼承 / 實作 / 體現**：`ProfessorTrapUmbrella` → `TransparentUmbrella` → `Item`；覆寫 [Template Method](../concepts/pat-template.md)。
- **每幀管線 / MVC 角色**：認領在 Collision / 互動階段執行。`kFlagHasProfessorTrap` 在 Ch2 / Ch3 / Ch4 進場時被各章漣漪函式查詢，形成跨章節的因果鏈。

## OO 概念與設計重點

[Template Method](../concepts/pat-template.md) 最重要的體現：「認領一把傘」的骨架流程（冪等守衛、記錄傘種、失活、發事件）在 `TransparentUmbrella` 定義，`ProfessorTrapUmbrella` 只覆寫「具體後果」——設 `kFlagHasProfessorTrap` 是這把傘獨有的副作用，其他傘不共享。[Observer](../concepts/pat-observer.md)：`UmbrellaClaimed` 事件接線到 GameController，後者從事件回呼產生追逐 NPC，`ProfessorTrapUmbrella` 本身不感知生成邏輯。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/entities/ProfessorTrapUmbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/entities/ProfessorTrapUmbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [Observer](../concepts/pat-observer.md)
