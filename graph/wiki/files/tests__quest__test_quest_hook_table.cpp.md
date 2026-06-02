---
id: file:tests/quest/test_quest_hook_table.cpp
type: test
path: tests/quest/test_quest_hook_table.cpp
domain: tests
bucket: quest
loc: 69
classes: []
sources: ["tests/quest/test_quest_hook_table.cpp"]
---
# `test_quest_hook_table.cpp`

> **一句定位**：驗證 E 互動任務 hook 的 `QuestHookTable`：表的大小（10 個）、每個 hook 的名稱順序與可呼叫性，以及不符合條件時整張表為無操作。

## 職責

此測試檔鎖定 `nccu::QuestHookTable` 的兩項契約，防止重構時靜默破壞 hook 順序或引入空 hook。

**契約一：順序與完整性**。`InteractQuestHooks()` 回傳的 vector 必須恰好 10 個，且按以下名稱順序排列：`TryReturnVictimUmbrella`、`TryReturnTaForm`、`TryRescueBookworm`、`TryMeetLibrarian`、`TryLendLibrarianUmbrella`、`TryReturnLibrarianUmbrella`、`TryApplyCh2Ripple`、`TryAdvanceCh3Trade`、`TryApplyCh3Ripple`、`TryApplyCh4Ripple`。順序具關鍵性——後面的 hook 可能讀到前面 hook 設下的旗標。

**契約二：可呼叫性**。每個 `QuestHook::fn` 都是有效的可呼叫物件（`static_cast<bool>(h.fn)` 為 true）。

**契約三：自我守門**。`RunInteractHooks` 對不符合的 `(npcId, state)` 走訪整張表不改動 karma、money、傘狀態。

**契約四：穩定單例**。`&InteractQuestHooks() == &InteractQuestHooks()` 確認每次取用同一個 vector 實例（無每幀重建）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::InteractQuestHooks()`：回傳穩定的 `std::vector<QuestHook>&`。
- `nccu::QuestHook`：含 `name`（`std::string_view`）和 `fn`（可呼叫物件）。
- `nccu::RunInteractHooks(EventBus, Player&, npcId, prevState, curState)`：走訪整張表的執行入口。
- `TEST_CASE("InteractQuestHooks：註冊順序與原始內嵌呼叫序列一致")`：10 個逐一 name 比對。
- `TEST_CASE("InteractQuestHooks：每筆項目都帶有可呼叫的函式物件")`。
- `TEST_CASE("RunInteractHooks：不符合的 (npcId, state) 不改動任何狀態")`。
- `TEST_CASE("RunInteractHooks：表是穩定的單例（同一份實例）")`。

## 相依與在架構中的位置

- **#include（往外）**：`EventBus.h`、`QuestHookTable.h`（`InteractQuestHooks`、`RunInteractHooks`）、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試每幀管線中 `RunInteractHooks` 呼叫點的行為（原本是 `GameController::Update` 中的多個 `TryXxx` 呼叫，現改為表驅動）。

## OO 概念與設計重點

`QuestHookTable` 是 [Command 模式](../concepts/pat-command.md)的應用：每個 `QuestHook` 封裝了一個操作（函式物件 `fn`）和名稱，`RunInteractHooks` 按順序執行。表驅動取代散落的 if-else 鏈，使測試能以 O(1) 複雜度驗證整個序列。「穩定單例」保證（`static local`）確保每幀呼叫 `InteractQuestHooks()` 不重建 vector，是效能與正確性的雙重保障。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_quest_hook_table.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_quest_hook_table.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Command](../concepts/pat-command.md)
