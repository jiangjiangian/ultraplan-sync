---
id: "file:tests/entities/test_cursed_taint.cpp"
type: test
path: tests/entities/test_cursed_taint.cpp
domain: tests
bucket: entities
loc: 103
classes: []
sources: ["tests/entities/test_cursed_taint.cpp"]
---
# `test_cursed_taint.cpp`

> **一句定位**：驗證 `CursedUmbrella` 的污染值機制——撿取遞增 `cursedTaint_`（當下 karma 不動）、`ApplyCursedTaintDecay` 依 `-5 * taint` 累進扣減、污染值在章節重置後仍保留，以及扣減受 karma 地板值 -100 裁切。

## 職責

本檔包含 4 個 `TEST_CASE`，完整釘住詛咒傘污染值機制的設計哲學（道德污點緩慢侵蝕而非一次爆發）。

**撿取遞增 taint、karma 不動**：新 Player（karma 起始 50）撿取 `CursedUmbrella` 後，`kFlagTookCursedUmbrella` 已設（Ending B 前置）、`HasUmbrella() == true`（背包有傘）、`karma == 50`（不動）、`cursedTaint == 1`。以 `BeClaimed` 再呼叫一次驗證冪等（`isActive_` 防止重複：taint 仍 1）。

**`ApplyCursedTaintDecay` 累進扣減**：
- `taint == 0` 時為空操作（karma 不動）。
- 每次 `IncCursedTaint()` 後 `ApplyCursedTaintDecay()`：taint 1 → `-5`；taint 2 → `-10`（累計 -15）；taint 3 → `-15`（累計 -30）。每次轉場的扣減量隨污染值遞增（`-5 * taint`，而非固定 -5）。

**污染值在章節重置後保留**：`SceneRouter` 進入新章節會呼叫 `SetHasUmbrella(false)` 清空背包的傘格；驗證此呼叫後 `cursedTaint == 1`（不清零）且 `kFlagTookCursedUmbrella` 仍保留——污染值與旗標是永久的道德記錄，不隨傘消失。

**karma 地板裁切**：先 `AddKarma(-150)` → 裁切為 -100；再 `IncCursedTaint()` + `ApplyCursedTaintDecay()`（未裁切會是 -105），驗證 `karma == -100`（地板阻止更低）。

## 關鍵內容（類別 / 函式 / 資料）

- `CursedUmbrella::BeClaimed(Player*)`：撿取入口，遞增 taint、設旗標、不動 karma。
- `Player::GetCursedTaint()`、`IncCursedTaint()`：污染值查詢與遞增。
- `Player::ApplyCursedTaintDecay()`：依 `-5 * taint` 扣 karma（受 -100 地板裁切）。
- `Player::SetHasUmbrella(false)`：清空背包傘格（不清 taint）。
- `nccu::kFlagTookCursedUmbrella`：Ending B 的前置旗標常數。
- `EventBus::Instance().Clear()`：每個 case 前後的清理。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/CursedUmbrella.h`、`include/game/entities/Player.h`、`include/engine/events/EventBus.h`、`include/engine/math/Vec2.h`、`include/game/quest/Flags.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

本檔釘住了 [Template Method](../concepts/pat-template.md) `BeClaimed` 在 `CursedUmbrella` 的特化語意（taint 遞增而非即時扣分），以及 `ApplyCursedTaintDecay` 的累進扣減設計。污染值的「永久保留」不變式（`SetHasUmbrella(false)` 不清零）是遊戲機制的設計保證：詛咒傘的道德代價不因傘消失而消失，只要持有過就會在每次進入新章節時「滲透」karma。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_cursed_taint.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_cursed_taint.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-template](../concepts/pat-template.md)
