---
id: file:tests/harness/test_scriptinput_classic_move.cpp
type: test
path: tests/harness/test_scriptinput_classic_move.cpp
domain: tests
bucket: harness
loc: 114
classes: []
sources: ["tests/harness/test_scriptinput_classic_move.cpp"]
---
# `test_scriptinput_classic_move.cpp`

> **一句定位**：守護迴歸：當腳本只含 classic 計時指令（無計畫動詞）時，`ResolvePlan(nullptr)` 不得釋放 `Advance()` 剛按住的鍵，且帶計畫動詞的腳本仍能正常流經解析器。

## 職責

此測試針對 `ScriptInput` 的 `Advance()` + `ResolvePlan()` 配對行為設立迴歸防護，驗證真正的 harness 執行順序：每幀先 `Advance()`（推進 classic 時間軸）、再 `ResolvePlan(snapshot)`（解析高階計畫動詞）、最後 `controller.Update()`（遊戲讀取輸入）。

關鍵迴歸：當 `plan_` 為空（純 classic 腳本）時，`ResolvePlan()` 不得把 WASD 的 `SynthUp` 廣播到空計畫——因為那會釋放 `Advance()` 剛按住的鍵，導致所有 classic WASD 移動失效。「提前返回」修正只應短路「無計畫」路徑，帶計畫的腳本不受影響。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ScriptInput：plan 為空時 classic 的 `down` 能在 ResolvePlan 後存活")`：純 classic 腳本 `"0 down D\n3 quit"`，確認 `HasPlan()==false`，逐格驗證 `Advance()/ResolvePlan(nullptr)` 配對後 D 鍵仍按住，press edge 完好，無錯誤 released；直到第 3 格 quit 觸發、D 仍按住。
- `TEST_CASE("ScriptInput：最小的計畫動詞腳本仍能正常解析")`：帶計畫的腳本 `"wait 2\nquit"`，透過完整的 Harness 迴圈（Advance/ResolvePlan/Update 各幀順序執行，快照落後一格），確認 `quitObserved`、`PlanDone()`——證明「提前返回」修正未把帶計畫路徑短路。

## 相依與在架構中的位置

- **#include（往外）**：`engine/platform/ScriptInput.h`（受測），`game/controller/GameController.h`，`engine/events/EventBus.h`，`game/world/World.h`，`game/dialog/DialogSource.h`，`engine/input/Input.h`，`engine/input/Key.h`，`engine/platform/Time.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（測試 harness 執行順序的迴歸防護）

## OO 概念與設計重點

此測試是一個精確的**迴歸測試**，守護「提前返回」修正的兩個副作用（計畫為空時的正確行為，以及帶計畫時不受影響）。`TEST_CONTENT_DIR` 的 `#error` 防護確保測試不在缺少對話資產的環境下靜默跳過。harness 執行順序（Advance→ResolvePlan→Update）被明確寫在測試程式碼的注釋與行為中，兼作架構說明文件。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/harness/test_scriptinput_classic_move.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/harness/test_scriptinput_classic_move.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
