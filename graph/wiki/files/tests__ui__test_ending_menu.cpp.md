---
id: "file:tests/ui/test_ending_menu.cpp"
type: test
path: tests/ui/test_ending_menu.cpp
domain: tests
bucket: ui
loc: 235
classes: [TestInput]
sources: ["tests/ui/test_ending_menu.cpp"]
---
# `test_ending_menu.cpp`

> **一句定位**：驗證結局畫面底部三選項選單（回首頁 / 重新開始 / 結束）的索引語意及 GameController 的端到端接線。

## 職責

本測試檔分兩層固定結局選單的行為：

**層 1（純索引語意）**：`EndingMenuChoiceAt(idx)` 把 0/1/2 分別對應到 `BackToTitle`、`RestartGame`、`Quit`。超出範圍的索引會夾進有效集合（-1→Quit、3→BackToTitle）。`World::kEndingMenuItemCount` 固定為 3，每個選項的標籤字串非空。

**層 2（GameController 接線）**：透過 `TestInput` 模擬輸入，驗證：
- 結局畫面上 ←/→ 會環狀移動 `World::EndingMenuCursor()`（0→1→2→0；0→2）。
- 按 Enter 或 E 確認後，游標 0 → `AppAction::Restart`；游標 1 → `AppAction::Restart`；游標 2 → `AppAction::Quit`。
- 結局畫面期間世界凍結：按住移動鍵 30 幀，玩家位置不變。

`TestInput` 是與 `test_pause_menu_toggle`、`test_menu_help` 共用的最小 `InputSource`，支援 `Hold/Release/Tap/EndFrame`。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：最小 `InputSource` 實作，用 `std::set<int>` 管理 `down_`、`pressed_`、`released_`、`autoUp_`。
- `Frame(controller, in)`：`controller.Update()` + `in.EndFrame()` 的組合步進函式。
- `EnterEnding(World&, SemesterState)`：強制轉場到指定結局狀態並 REQUIRE 其生效。
- `EndingMenuChoiceAt(int)` — 被測：索引→枚舉對應。
- `EndingMenuLabel(EndingMenuChoice)` — 被測：各選項的中文標籤字串。
- `World::EndingMenuCursor()` / `World::PendingAppAction()` — 被測觀察目標。

## 相依與在架構中的位置

- **#include（往外）**：`game/controller/GameController.h`（驅動 Update）、`game/world/World.h`（讀取游標與 AppAction）、`ui/EndingView.h`（EndingMenuChoiceAt / EndingMenuLabel）、`engine/input/Input.h`（TestInput 基底）、`engine/platform/Time.h`（SetFixedStep）、`engine/events/EventBus.h`（清空用）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 實作 `include/engine/input/Input.h`。
- **每幀管線 / MVC 角色**：Controller 層測試，驗證結局態下 GameController 的凍結機制與選單路由。

## OO 概念與設計重點

藉由 `TestInput`（`InputSource` 的 [Strategy 模式](../concepts/pat-strategy.md)替身）替換 LiveInput，在不開窗口的情況下端到端驗證 [GameController](../concepts/arch-mvc.md) 的 Controller 路徑。體現了 [arch-harness](../concepts/arch-harness.md) 的核心設計：固定步進時間 + 可替換輸入源。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_ending_menu.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_ending_menu.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Harness](../concepts/arch-harness.md) · [Strategy](../concepts/pat-strategy.md)
