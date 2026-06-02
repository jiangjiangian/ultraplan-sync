---
id: file:tests/quest/test_ch4_finale.cpp
type: test
path: tests/quest/test_ch4_finale.cpp
domain: tests
bucket: quest
loc: 127
classes: []
sources: ["tests/quest/test_ch4_finale.cpp"]
---
# `test_ch4_finale.cpp`

> **一句定位**：驗證 Ch4 助教結算選單（體諒／質問／退出）的三個選項屬性及副作用，並端到端確認體諒路徑如何延遲到收尾台詞結束後才結算到 Ending A。

## 職責

此測試檔屬於 `tests/quest/` 桶，針對 `SemesterState::Chapter4_Finals` 的助教最終對話邏輯進行單元測試。它完全繞過 `GameController`，直接操作 `DialogState`、`Player`、`SemesterStateMachine` 與 `EventBus`，驗證在純資料層面上各選項的正確性。

核心驗證場景有四個：第一，選單呈現時恰好有三個選項（索引 0 體諒、索引 1 質問、索引 2 退出），且退出項帶有 `karmaDelta==0` 且 `setsFlag` 為空；第二，質問分支（索引 1）返回 `-5` karma 且不設旗標；第三，一旦玩家旗標 `kFlagTaFinaleChoiceMade` 已設，再對話只剩純台詞重述而非選單；第四，體諒 + `kFlagHasTrueUmbrella` + karma 充足時，`CheckEndingGates` 在對話框仍開啟時會延後，待 `d.Close()` 後才轉到 `Ending_A`。

測試引用了真實的內容目錄 `TEST_CONTENT_DIR`（編譯期必須定義），透過 `nccu::dialog::SetContentDir` 載入對話資料，藉此驗證整條解析到旗標設定的管線都正確連通。本地輔助函式 `StepToChoice` 最多推進 64 步來跳過開場台詞，直到 `d.AtChoice()` 為止。

## 關鍵內容（類別 / 函式 / 資料）

- `MakePlayer()`：匿名 namespace 的 helper，建立位於原點的 `Player`。
- `StepToChoice(nccu::DialogState&)`：持續呼叫 `d.Advance()` 直到 `d.AtChoice()` 或 64 次守衛為止。
- `TEST_CASE("Ch4 助教結算：呈現程式建構的「體諒／質問」選單")`：斷言三個選項的標籤、退出項的零副作用、體諒項的 `karmaDelta==15` 與 `setsFlag==kFlagConsoledTA`。
- `TEST_CASE("質問分支是 -5 karma、不設旗標的路徑")`：以 `d.MoveChoice(1)` 選質問，確認返回 `-5` 與空旗標。
- `TEST_CASE("Flag_TaFinaleChoiceMade -> 純台詞重述（單次）")`：預設旗標後對話絕不出現 `AtChoice()`。
- `TEST_CASE("體諒選擇端到端走完 Ending A 路徑")`：模擬 `GameController::ApplyDialogChoice`（手動加 karma、設旗標），再呼叫 `CheckEndingGates` 兩次，驗證對話中延後、關閉後轉 `Ending_A`。

## 相依與在架構中的位置

- **#include（往外）**：`EventBus.h`（發布 / 清除事件）、`Vec2.h`（Player 建構）、`DialogOpener.h`（`OpenNpcDialog`）、`DialogSource.h`（`SetContentDir`）、`DialogState.h`（`Advance/AtChoice/Choices/Close`）、`Player.h`（karma/flag 操作）、`Flags.h`（旗標常數）、`EndingGate.h`（`CheckEndingGates`）、`SemesterStateMachine.h`（狀態機）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 純單元測試，不在每幀管線中；測試對象是 CheckEndingGates（每非對話幀輪詢的結局判定點）。

## OO 概念與設計重點

本測試體現 doctest 單元測試風格，核心設計特點是「分兩次輪詢」的延遲判定驗證：對話框開啟時 `CheckEndingGates` 是 no-op，關閉後才落地結局，精確對應了 [State 模式](../concepts/pat-state.md)下 `SemesterStateMachine` 的不可變性（測試前後都呼叫 `EventBus::Instance().Clear()` 以維持測試隔離）。`StepToChoice` 使用守衛上限 64 反映了「測試不應無限等待」的防禦性設計。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_finale.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_finale.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md)
