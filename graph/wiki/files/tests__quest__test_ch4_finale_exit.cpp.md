---
id: file:tests/quest/test_ch4_finale_exit.cpp
type: test
path: tests/quest/test_ch4_finale_exit.cpp
domain: tests
bucket: quest
loc: 151
classes: [TestInput]
sources: ["tests/quest/test_ch4_finale_exit.cpp"]
---
# `test_ch4_finale_exit.cpp`

> **一句定位**：透過真正的 `GameController::Update()` 迴圈與自訂輸入源 `TestInput`，端到端驗證「退出助教結算選單」完全零副作用且事後可重新開啟。

## 職責

此測試檔是整個批次中整合程度最高的一個：它不使用純資料墊片，而是建構真實的 `World`、`GameController`，並注入自訂的 `TestInput`（繼承自 `InputSource`）來驅動按鍵序列，走過與正式遊玩完全相同的確認路徑。測試目標是 Ch4 助教結算選單的「退出」項（`kDialogExitLabel`）。

關鍵不變量：選退出後 `kFlagTaFinaleChoiceMade` 不得被設下、`kFlagConsoledTA` 不得被設下、karma 不得改變、學期仍停留 `Chapter4_Finals`。之後再次靠近助教開啟對話，同樣的三個選項必須重新出現——證明「退出」只是延後決定而非永久鎖死。

`TestInput` 提供 `Hold`/`Release`/`Tap`（一幀自動放開）和 `EndFrame()`（清除 pressed/released 並處理 autoUp），精確模擬輸入幀語義。輔助函式 `Frame(GameController&, TestInput&)` 每次呼叫 `controller.Update()` 後清除輸入幀。`AdvanceToChoice` 最多迴圈 24 幀連按 E 跳過開場台詞。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`（繼承 `nccu::engine::input::InputSource`）：管理 `down_/pressed_/released_/autoUp_` 四個 `std::set<int>`，實作 `IsDown/IsPressed/IsReleased`；`Tap` 會在 `EndFrame` 時自動釋放。
- `Frame(GameController&, TestInput&)`：調用 `c.Update()` 再呼叫 `in.EndFrame()`，對應一個遊戲幀。
- `FindNpc(const World&, const char*)` ：在 `w.Objects()` 中找到 isActive 且 NpcId 匹配的物件。
- `AdvanceToChoice`：最多 24 幀連按 E 直到 `w.Dialog().AtChoice()`。
- `TEST_CASE("拒絕助教結局（我再想想…）不改動任何狀態且可重新開啟")`：完整端到端場景，包含前置高 karma 與 `kFlagHasTrueUmbrella` 設定、把游標移到退出項、確認後驗證零副作用、再次開啟確認同樣的三個選項。

## 相依與在架構中的位置

- **#include（往外）**：`GameController.h`（`Update` 驅動）、`World.h`（持有對話/玩家/物件）、`Player.h`、`GameObject.h`、`DialogState.h`（`AtChoice/Choices/ChoiceCursor`）、`DialogSource.h`（`SetContentDir`）、`EventBus.h`、`SemesterState.h`、`Input.h`（`SetSource`）、`Key.h`（按鍵列舉）、`Time.h`（`SetFixedStep`）、`Flags.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：`TestInput` 繼承 `include/engine/input/Input.h`（`InputSource`）。
- **每幀管線 / MVC 角色**：測試驅動完整的 Controller 更新管線（Survival→Sweep），驗證的是 Controller 在對話確認路徑上的行為。

## OO 概念與設計重點

`TestInput` 是 [Strategy 模式](../concepts/pat-strategy.md)的典型應用：`InputSource` 是抽象策略，`TestInput` 是測試替身，透過 `Input::SetSource` 注入，讓測試在無 raylib 環境下可控地提供按鍵序列。測試結束時呼叫 `Input::SetSource(nullptr)` 和 `Time::SetFixedStep(0.0f)` 清除全局狀態，體現了對測試隔離的重視（[RAII](../concepts/oo-raii.md) 式手動清理）。整體符合 [Harness 架構](../concepts/arch-harness.md)的設計意圖。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_finale_exit.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_finale_exit.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [Harness](../concepts/arch-harness.md)
