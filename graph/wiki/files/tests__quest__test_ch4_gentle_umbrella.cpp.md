---
id: file:tests/quest/test_ch4_gentle_umbrella.cpp
type: test
path: tests/quest/test_ch4_gentle_umbrella.cpp
domain: tests
bucket: quest
loc: 137
classes: []
sources: ["tests/quest/test_ch4_gentle_umbrella.cpp"]
---
# `test_ch4_gentle_umbrella.cpp`

> **一句定位**：驗證溫柔結局路徑中 `TryGrantTaFinaleUmbrella` 的授傘邏輯：體諒→授傘冪等性、強硬質問不授傘，以及與 Ending A/B/C/D 判定的完整端到端對照。

## 職責

此測試檔集中於 `nccu::TryGrantTaFinaleUmbrella` 這個任務層輔助函式——它由 `GameController` 在確認助教結算選擇後呼叫，負責在體諒路徑（`kFlagConsoledTA`）下無條件授予 `kFlagHasTrueUmbrella` 與實際持傘狀態。

核心設計驗證：
1. 體諒旗標設下時授予傘，且第二次呼叫具冪等性（不重複設旗）。
2. 強硬質問路徑（有 `kFlagTaFinaleChoiceMade` 但無 `kFlagConsoledTA`）調用後無效果。
3. 對象不符（`"victim"` 而非 `"ta"`）或章節不符（非 `Chapter4_Finals`）時也無效果。
4. 端到端：體諒 + karma>80 + 授傘後 → `Ending_A`；體諒但 karma≤80 → `Ending_D`；質問路徑（高 karma 但無體諒）→ `Ending_B`。
5. 既有的 Ending B（詛咒傘）與 Ending C（醜傘）路徑不受新輔助函式影響。

所有 TEST_CASE 都是純資料層（無 `GameController`），直接設旗標再呼叫 `CheckEndingGates`，精確對應 GameController 確認時的實際操作順序。

## 關鍵內容（類別 / 函式 / 資料）

- `MakePlayer()`：建構原點 `Player` 的匿名 namespace helper。
- `TEST_CASE("體諒（Flag_ConsoledTA）授予 Flag_HasTrueUmbrella 與 HasUmbrella")`：授予 + 冪等性。
- `TEST_CASE("強硬或尚未做的結局都拿不到傘")` + SUBCASE：質問路徑與對象/章節不符的守門。
- `TEST_CASE("體諒 + karma>80 不需隱藏傘即可抵達 Ending A")`：依 GameController 的確認順序：`ApplyDialogChoice` → 授傘 → `CheckEndingGates` → `Ending_A`。
- `TEST_CASE("體諒但 karma<=80 的玩家仍拿到傘（-> Ending D，不卡關）")`：低 karma 體諒仍授傘，落到 `Ending_D`。
- `TEST_CASE("強硬質問（冷漠結局）仍導向 Ending B 而非 A")`：高 karma 無體諒，授傘無效，`Ending_B`。
- `TEST_CASE("溫柔傘的改動不影響既有的 Ending B/C")`：SUBCASE 詛咒傘 → B、醜傘 → C。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`Chapter4Quest.h`（`TryGrantTaFinaleUmbrella`）、`EventBus.h`、`EndingGate.h`（`CheckEndingGates`）、`SemesterStateMachine.h`、`DialogState.h`、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層的旗標邏輯，模擬 GameController 在確認選擇後的操作序列。

## OO 概念與設計重點

本檔體現「防卡關不變量」的測試哲學：每個 SUBCASE 都在某條邊界條件下（karma 邊界 80、對象不符、章節不符）驗證系統不會留下不可退出的狀態。`TryGrantTaFinaleUmbrella` 的守門設計（多條件 `&&` 檢查）符合 SOLID 的單一職責原則。授傘發生在 `CheckEndingGates` 之前的順序是個容易回歸的細節，本測試精確釘住此操作順序。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_gentle_umbrella.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_gentle_umbrella.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
