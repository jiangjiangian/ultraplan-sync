---
id: "file:tests/ui/test_hud_reset.cpp"
type: test
path: tests/ui/test_hud_reset.cpp
domain: tests
bucket: ui
loc: 76
classes: []
sources: ["tests/ui/test_hud_reset.cpp"]
---
# `test_hud_reset.cpp`

> **一句定位**：驗證 `World::HudExpired()` 述詞的語意——HUD 提示超過 TTL 後轉為過期、重新發佈會重置、空訊息不算過期。

## 職責

本測試修正了一個隱藏缺陷：`DrawHudMessage` 在 `hudAge >= kHudTtl` 時提前返回（View 正確停止繪製），但輸出 `state.jsonl` 的自動遊玩工具仍看到 `HudMessage()` 的非空字串，因為舊版本從未清空字串緩衝。

`HudExpired()` 是新增的**唯讀述詞**，不改變狀態，讓工具以它而非 `HudMessage().empty()` 來判斷「不再顯示」。測試驗證：
1. 當 `hudAge_` 超過 `kHudTtl`（約 4 秒，用 41 × 0.1f 確保浮點不穩定下穩定越界）後，`HudExpired()` 為 true，但 `HudMessage()` 字串仍保留（View 的淡出動畫需要它）。
2. 重新呼叫 `SetHudMessage` 會把存活時間歸零並清除過期狀態（HudAge → 0.0f，HudExpired → false）。
3. 從未設定提示的 World 在超過 TTL 後 `HudExpired()` 仍為 false（空字串沒有東西可過期）。

## 關鍵內容（類別 / 函式 / 資料）

- `World::HudExpired()` — 被測：返回 `hudAge_ >= kHudTtl && !hudMessage_.empty()` 的語意。
- `World::TickHud(float dt)` — 配合：推進 `hudAge_`。
- `World::SetHudMessage(string)` — 配合：更新訊息並重置 `hudAge_`。
- `World::HudMessage()` / `World::HudAge()` — 觀察目標。
- `nccu::kHudTtl` — 被引用：TTL 常數，由 `MessageView.h` 匯出。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/World.h`（Model）、`ui/MessageView.h`（匯出 `kHudTtl`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試，驗證 World 的 HUD 狀態管理。

## OO 概念與設計重點

此測試體現了「工具/View 共用同一狀態來源，但以不同述詞觀察」的關注點分離設計：`HudExpired()` 是面向工具的述詞，`HudMessage()`/`HudAge()` 面向 View 的淡出動畫，兩條路徑的契約彼此不衝突。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_hud_reset.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_hud_reset.cpp) · [← 全檔索引](../files-index.md)
