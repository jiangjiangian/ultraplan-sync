---
id: "file:include/game/controller/InputHandler.h"
type: header
path: include/game/controller/InputHandler.h
domain: game
bucket: controller
loc: 75
classes: [InputHandler]
sources: ["include/game/controller/InputHandler.h"]
---
# `InputHandler.h`

> **一句定位**：從 `GameController` 抽出的跨幀輸入計時類別——專責「按住 E 自動推進對話」的邊緣計時邏輯，讓 Controller 保持 orchestrator 身份而非膨脹成 god method。

## 職責

`InputHandler` 是 game controller 層的輸入計時器，目前只有一項跨幀狀態：「按住 E 自動推進對話」的計時與冷卻。其餘所有無狀態的輸入讀取（`IsPressed`/`IsDown`/`IsReleased`）留在各自的呼叫點，不需集中。

`TickDialogAdvance(float dt)` 是核心方法：回傳本幀是否應「推進對話」。觸發條件為以下任一：E 鍵邊緣觸發（`IsPressed(E)`）；或連續按住 E 達 `kHoldAdvanceMs`（300 ms）且冷卻已歸零（每次自動觸發後重設 `kAutoCooldownFrames`（4 幀）冷卻）。這讓玩家能逐頁閱讀，而非以每秒 60 頁的速度閃過。方法在同一 tick 內冪等（Controller 每幀至多呼叫一次）。

`ResetDialogAdvance()` 在對話框關閉（或本幀從未開啟）時呼叫，丟棄累積的按住計時——避免「對話框出現前累積的 300 ms 在新對話第一幀就自動觸發」。

兩個公開的 `constexpr` 常數（`kHoldAdvanceMs`/`kAutoCooldownFrames`）作為測試接縫，讓回歸測試可精確驗證邊緣/冷卻語意而不依賴魔法數字。兩個測試專用 getter（`HoldMs()`/`Cooldown()`）允許白箱驗證計時器內部狀態。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class InputHandler` | 持有 `eHoldMs_`（E 已按住毫秒）和 `eAutoAdvanceCooldown_`（自動推進冷卻幀數）。 |
| `TickDialogAdvance(float dt)` → `bool` | 本幀是否推進對話；邊緣觸發或按住超過 300ms 且冷卻歸零。 |
| `ResetDialogAdvance()` | 清零 `eHoldMs_` 和 `eAutoAdvanceCooldown_`；對話關閉時呼叫。 |
| `kHoldAdvanceMs = 300.0f` | 測試接縫：按住觸發閾值（毫秒）。 |
| `kAutoCooldownFrames = 4` | 測試接縫：每次自動觸發後的靜默幀數。 |
| `HoldMs()` | 測試用白箱 getter：當前按住毫秒數。 |
| `Cooldown()` | 測試用白箱 getter：剩餘冷卻幀數。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/input/Input.h`（`IsPressed`/`IsDown` 等邊緣偵測 API）、`include/engine/input/Key.h`（E 鍵常數）。
- **被誰使用（往內）**：`include/game/controller/GameController.h`（作為成員 `input_`）、`src/game/controller/InputHandler.cpp`（`TickDialogAdvance` 實作）、`src/game/controller/screens/DialogScreen.cpp`（對話畫面每幀呼叫 `TickDialogAdvance`）、`tests/controller/test_input_handler.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層的輸入子系統；在每幀的 `HandleDialog` 階段被呼叫，不參與模擬管線本身（管線只操作 Model）。

## OO 概念與設計重點

`InputHandler` 展現了**單一職責原則（SRP）**的精確應用：把「按住 E 計時」這一個跨幀狀態從 `GameController` 的龐大更新方法中抽出，賦予它獨立的生命週期和可測試性。`constexpr` 常數作為測試接縫（可在測試中直接引用 `InputHandler::kHoldAdvanceMs` 而非魔法數字），是一種輕量級的「可配置邊界」設計。

純輸入計時（不變動 World、不發布事件）的設計讓 `doctest` 可以餵入合成的 `InputSource` 並斷言精確的邊緣/冷卻語意，無需啟動完整 `GameController`。這是測試驅動設計（TDD）分離關注點的具體成果。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/InputHandler.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/InputHandler.h) · [← 全檔索引](../files-index.md)
