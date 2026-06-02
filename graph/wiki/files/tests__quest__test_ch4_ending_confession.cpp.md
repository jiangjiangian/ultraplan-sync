---
id: file:tests/quest/test_ch4_ending_confession.cpp
type: test
path: tests/quest/test_ch4_ending_confession.cpp
domain: tests
bucket: quest
loc: 188
classes: [TestInput, Ch4Fixture]
sources: ["tests/quest/test_ch4_ending_confession.cpp"]
---
# `test_ch4_ending_confession.cpp`

> **一句定位**：透過真正的 `GameController::Update()` 迴圈，端到端驗證 Ch4 結局的「內心自白延後」——自白播放期間結局不轉場，對話關閉後才結算，且自白依優先序只觸發一次。

## 職責

此測試驗證「結局不可突兀」的設計：每個 Ch4 結局在轉場前，會先開啟一段內心獨白（自白），玩家讀完後結局才觸發。測試透過真正的 GameController 迴圈驗證這個延後機制的端到端行為。

`Ch4Fixture` 封裝了 Ch4 測試環境的建立與拆卸：建立 World、接上 GameController、設定固定 timestep、轉場到 Ch4、先跑一幀讓進場副作用沉澱（清除傘和旗標），之後才武裝玩家狀態。

`TryOpenEndingConfession` 的單元測試直接驗證函式的所有邊界條件：非 Ch4 無操作、對話框開著無操作、once-key 擋住重複開啟、詛咒優先於醜傘（與 B 優先於 C 的閘門一致）。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：全鍵 false 的最小 `InputSource` stub，讓玩家站著不動，只跑每幀的雨/自白/閘門邏輯。
- `Ch4Fixture`：RAII 型測試夾具，持有 `World`、`GameController`、`TestInput`；建構子設定環境，解構子 teardown（`SetSource(nullptr)`、`Time::SetFixedStep(0)`、`EventBus.Clear()`）；提供 `P()` 存取器。
- `Frame(controller, in)`：執行一幀的輔助函式。
- `TEST_CASE("買醜傘結局延後到自白之後，關閉後才結算為 Ending C")`：設 `kFlagBoughtUglyUmbrella`→下一幀出現自白→多幀不轉場→關閉→下一幀轉場 `Ending_C`。
- `TEST_CASE("詛咒傘結局延後到自白之後，關閉後才結算為 Ending B")`：同樣流程，旗標改 `kFlagTookCursedUmbrella`，結局為 `Ending_B`。
- `TEST_CASE("自白具單次性（once-key）— 讀過後絕不重新開啟")`：設 `kFlagCh4ConfessedUgly` 後再跑不再觸發自白。
- `TEST_CASE("TryOpenEndingConfession 依優先序恰好開啟一段自白一次")`：四個 SUBCASE——非 Ch4 無操作、詛咒優先於醜傘、對話開著不打斷/once-key 擋住、真傘自白的條件（`kFlagTaFinaleChoiceMade` 抑制）。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter4Quest.h`，`game/state/EndingGate.h`，`game/controller/GameController.h`，`game/world/World.h`，`game/entities/Player.h`，`game/dialog/DialogState.h`、`DialogSource.h`，`engine/events/EventBus.h`，`game/state/SemesterState.h`，`engine/input/Input.h`、`Key.h`，`engine/platform/Time.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 繼承 `InputSource`（`include/engine/input/Input.h`）。
- **每幀管線 / MVC 角色**：直接驗整個 Ch4 每幀管線中的「自白→閘門」延後機制（CheckEndingGates 在每非對話幀輪詢）。

## OO 概念與設計重點

`Ch4Fixture` 是 RAII 測試夾具的完整示範：複雜的環境建立集中在建構子，拆卸在解構子，使每個 TEST_CASE 只需宣告 `Ch4Fixture fx;` 即可獲得乾淨的 Ch4 環境。`TryOpenEndingConfession` 的 once-key 機制（`kFlagCh4ConfessedX`）防止自白在同一幀被多個觸發路徑重複開啟，體現了冪等設計原則。真傘自白被 `kFlagTaFinaleChoiceMade` 抑制的測試則確保結局選擇路徑的差異行為被正確釘住。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_ending_confession.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_ending_confession.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [RAII](../concepts/oo-raii.md) · [Harness](../concepts/arch-harness.md)
