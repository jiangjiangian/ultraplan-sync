---
id: "file:tests/ui/test_menu_help.cpp"
type: test
path: tests/ui/test_menu_help.cpp
domain: tests
bucket: ui
loc: 260
classes: [TestInput]
sources: ["tests/ui/test_menu_help.cpp"]
---
# `test_menu_help.cpp`

> **一句定位**：驗證暫停選單說明項的完整行為——透過 GameController 真實輸入迴圈確認說明遮罩的開啟/關閉、分頁翻頁與模擬凍結，以及說明文字的內容契約。

## 職責

本測試針對暫停選單新增的「說明」項（索引 1，6 列選單中固定不變）進行端到端測試。以 `TestInput`（`InputSource` 替身）驅動 `GameController::Update()`，不需真實視窗。

**主測試 case**（`暫停選單的說明開啟／關閉說明遮罩，模擬凍結`）驗證：
- `World::kMenuItemCount == 6`（含新增的減少動畫/擴大目標切換列）。
- M 鍵開選單；Down 移到索引 1（說明）；Enter 開啟 `World::HelpOpen()`，選單仍在後面開著，未請求 AppAction。
- 說明開著時按住移動鍵 D 30 幀，玩家座標不變、雨量不前進（凍結）；D 鍵不關說明。
- E 鍵把說明關回選單（`HelpOpen` → false，MenuOpen 仍 true，游標保持 1）。
- M 鍵從選單恢復遊戲，`SetMenuOpen(false)` 同時清掉說明鎖存。
- `kGameHelpLines` 非空且每行字元數 ≤ 24；含「暫停」與「雨壓力」同行的提示行。

**分頁測試 case**（`GameHelp 被拆成兩個一致的分頁`）驗證：
- `kGameHelpPageCount == 2`；分頁攤平後與 `kGameHelpLines` 逐項相等；含道具說明提示（跨章節/清空/減緩雨量/自動減緩）。

**翻頁測試 case**（`←／→ 翻動說明遮罩`）驗證：
- 開啟時頁碼重置為 0；→ 前進到頁 1；再 → 環繞回頁 0；← 反向環繞；翻頁期間模擬凍結；關閉再重開頁碼回到 0；從選單恢復遊戲時 `HelpPage` 重置。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：與 `test_pause_menu_toggle` / `test_ending_menu` 共用設計的最小 `InputSource`，支援 `Hold/Release/Tap/EndFrame`。
- `Frame(GameController&, TestInput&)`：`Update()` + `EndFrame()` 的步進函式。
- `World::HelpOpen()` / `World::HelpPage()` / `World::MenuOpen()` / `World::MenuCursor()` — 觀察目標。
- `nccu::kGameHelpLines` / `nccu::kGameHelpPages` / `nccu::kGameHelpPageCount` / `nccu::kGameHelpClosing` — 被測內容常數。

## 相依與在架構中的位置

- **#include（往外）**：`game/controller/GameController.h`、`game/world/World.h`、`ui/GameHelp.h`、`game/entities/Player.h`、`engine/input/Input.h`（TestInput 基底）、`engine/platform/Time.h`、`engine/events/EventBus.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 實作 `include/engine/input/Input.h`。
- **每幀管線 / MVC 角色**：Controller 層測試，驗證暫停/說明狀態對遊玩凍結的正確封鎖。

## OO 概念與設計重點

藉由 [Harness](../concepts/arch-harness.md) 的固定步進時間 + `TestInput` [Strategy 替身](../concepts/pat-strategy.md)，在不開窗口的情況下端到端驗證 [MVC Controller](../concepts/arch-mvc.md) 路徑。說明分頁的一致性測試（攤平等於扁平清單）確保字形掃描的 `kGameHelpLines` 不會與渲染器實際繪製的 `kGameHelpPages` 脫鉤。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_menu_help.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_menu_help.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Harness](../concepts/arch-harness.md) · [Strategy](../concepts/pat-strategy.md)
