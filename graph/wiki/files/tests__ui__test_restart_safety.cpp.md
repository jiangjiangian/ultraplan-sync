---
id: "file:tests/ui/test_restart_safety.cpp"
type: test
path: tests/ui/test_restart_safety.cpp
domain: tests
bucket: ui
loc: 142
classes: []
sources: ["tests/ui/test_restart_safety.cpp"]
---
# `test_restart_safety.cpp`

> **一句定位**：驗證遊戲重啟的兩個安全不變量——玩家狀態完全歸零（無跨輪污染），以及 EventBus 訂閱者不懸空也不重複堆疊。

## 職責

本測試模擬 `main.cpp` 的真實重啟路徑：建立 `{World, GameController}` 範圍 → 弄髒 → RAII 銷毀（`~GameController` 呼叫 `EventBus::Clear()`，早於 `~World`）→ 建立新範圍。

**狀態歸零測試**（第 1 輪）：`DirtyTheRun()` 把玩家改成 money=250、karma=10、設兩個旗標、囤 2 個 HotPack、`SetHasUmbrella(true)`。第 1 輪 RAII 銷毀後，第 2 輪 World 驗證：karma=50（企劃起始值）、money=100、rainMeter=0.0f、no umbrella、旗標皆清、HotPack count=0、學期回到 `Chapter1_AddDrop`。

**EventBus 訂閱者安全測試**（4 輪迴圈）：每輪建立 `{World, GameController}` + 探針訂閱者；發佈 `ShowMessage("restart-cycle-N")`；驗證：
- 現存 World 的 HUD 訂閱者有觸發（`HudMessage() == msg`）——處理器存在且指向正確的 World。
- 探針恰好觸發一次（`probeHits == 1`）——無前輪殘留或重複訂閱。
  
最後驗證 4 輪 `GameController` 全部解構後，再加的探針是唯一存活的訂閱者（`afterAll == 1`）。

## 關鍵內容（類別 / 函式 / 資料）

- `DirtyTheRun(World&)`：把 Player 弄成「弄髒的」進行中狀態（money+、karma-、旗標、消耗品、傘）。
- `runOneCycleAndCount(int cycle)`：Lambda，封裝一輪的建立/訂閱/發佈/斷言/RAII 銷毀。
- `World::GetPlayer()` / `Player::GetKarma()` / `GetMoney()` / `GetRainMeter()` / `HasUmbrella()` / `HasFlag()` / `ConsumableCount()` — 被驗證的清潔狀態。
- `World::HudMessage()` — 被驗證的訂閱者存活證明。
- `GameController` 解構子 / `EventBus::Clear()` — 被測的清理機制。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/World.h`、`game/controller/GameController.h`、`game/entities/Player.h`、`engine/events/EventBus.h`、`game/state/SemesterState.h`、`game/quest/Flags.h`、`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：整合層測試，驗證 Model + Controller 的 RAII 生命週期管理。

## OO 概念與設計重點

本測試直接對應 [RAII](../concepts/oo-raii.md) 模式的實踐——`GameController` 的解構子負責清空 EventBus，確保對已銷毀 World 的捕捉不會成為懸空參考。多輪迴圈固定「無累積」不變量，是防止 [Observer](../concepts/pat-observer.md) 訂閱洩漏的關鍵閘。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_restart_safety.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_restart_safety.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md) · [Observer](../concepts/pat-observer.md)
