---
id: "file:tests/ui/test_pause_menu_toggle.cpp"
type: test
path: tests/ui/test_pause_menu_toggle.cpp
domain: tests
bucket: ui
loc: 331
classes: [TestInput]
sources: ["tests/ui/test_pause_menu_toggle.cpp"]
---
# `test_pause_menu_toggle.cpp`

> **一句定位**：透過 GameController 真實輸入迴圈，端到端驗證 6 列暫停選單的游標環狀走訪、減少動畫/擴大目標的就地切換、列位移後破壞性列的 AppAction 路由，以及選單只開於 M 不開於 ESC。

## 職責

本測試用 `TestInput` 驅動 `GameController::Update()`，端到端固定暫停選單在從 4 列擴增為 6 列後的所有邊界條件。

**游標環狀走訪**：`kMenuItemCount == 6`；↓ 走訪 0→5，再 ↓ 環繞回 0；↑ 從 0 環繞到 5。

**減少動畫切換（索引 2）**：移到第 2 列按 Enter → `World::ReducedMotion()` 翻轉為 true，選單保持開啟、游標停在 2；再按 Enter 還原為 false（往復切換）；全程 `LargeTargets` 不受影響。

**擴大目標切換（索引 3）**：移到第 3 列按 Enter → `World::LargeTargets()` 翻轉；再按 Enter 還原；全程 `ReducedMotion` 不受影響（兩個切換互相獨立）。

**列位移後 AppAction**：重新開始由索引 2 → 4，離開由 3 → 5。第 4 列 Enter → `AppAction::Restart`；第 5 列 Enter → `AppAction::Quit`。兩個切換旗標不受影響。

**M vs ESC 選單鍵**：ESC 不開選單、不關選單（只有 M 控制）。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：與多個 UI 測試共用設計的最小 `InputSource`，管理 `down_`/`pressed_`/`released_`/`autoUp_` 四個集合。
- `Frame(GameController&, TestInput&)`：步進函式。
- `World::kMenuItemCount` — 被測常數（== 6）。
- `World::MenuCursor()` / `World::MenuOpen()` / `World::ReducedMotion()` / `World::LargeTargets()` / `World::PendingAppAction()` — 觀察目標。

## 相依與在架構中的位置

- **#include（往外）**：`game/controller/GameController.h`、`game/world/World.h`、`game/entities/Player.h`、`engine/input/Input.h`（TestInput 基底）、`engine/platform/Time.h`、`engine/events/EventBus.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 實作 `include/engine/input/Input.h`。
- **每幀管線 / MVC 角色**：Controller 層測試，覆蓋 GameController 的暫停選單路由分支。

## OO 概念與設計重點

以 [Harness](../concepts/arch-harness.md) 固定步進時間 + `TestInput` [Strategy 替身](../concepts/pat-strategy.md)，不需真實視窗即可完整走過游標邊界。兩個無障礙旗標獨立性測試與 `test_large_targets` / `test_reduced_motion` 形成互補（後者測原始 setter，本檔測 UI 接線）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_pause_menu_toggle.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_pause_menu_toggle.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Harness](../concepts/arch-harness.md) · [Strategy](../concepts/pat-strategy.md)
