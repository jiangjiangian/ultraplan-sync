---
id: file:include/game/quest/QuestHookTable.h
type: header
path: include/game/quest/QuestHookTable.h
domain: game
bucket: quest
loc: 89
classes: [QuestHook]
sources: ["include/game/quest/QuestHookTable.h"]
---
# `QuestHookTable.h`

> **一句定位**：把「按 E 互動」的各章任務鉤子資料化為有序表格，消除 `GameController::Update` 中硬編碼的 `TryXxx` 呼叫序列，是 Controller 互動管線的核心分派入口。

## 職責

`QuestHookTable.h` 宣告統一鉤子簽章 `QuestHookFn`、鉤子登記結構體 `QuestHook`，以及兩個自由函式 `InteractQuestHooks()`（取有序鉤子表）和 `RunInteractHooks()`（執行整張表）。

設計動機明確：原本 `GameController::Update` 中約 14 個內聯的 `TryXxx(npcId, state)` 呼叫（`TryReturnVictimUmbrella`、`TryRescueBookworm`、`TryMeetLibrarian`、`TryLendLibrarianUmbrella`、`TryReturnLibrarianUmbrella`、`TryApplyCh2Ripple`、`TryAdvanceCh3Trade`、`TryApplyCh3Ripple`、`TryApplyCh4Ripple` 共 9 個主要鉤子）在一張表中登記，使「新增章節/NPC 鉤子」成為資料而非修改控制器。這是**開放封閉原則（OCP）**的直接應用。

`QuestHookFn` 統一三種原始簽章（帶 bus 與否、帶 returnTo 與否、帶 npcId 與否）為同一個 5-參數 lambda 包裹，使整張表同質。`returnTo`（`World::Semester().InterludeReturnTo()`）在每次呼叫時轉發，供 `TryReturnLibrarianUmbrella` 等需要幕間返回目標的鉤子使用。

每個鉤子內部自我閘控於 `(state, npcId)`，整張表從頭走到尾，只有條件相符的鉤子才實際執行（其餘為廉價 no-op）。登記順序「完全對齊原本的內聯呼叫序列」，確保跨鉤子的旗標讀寫依序（例如管理員相遇旗標先設後讀）。

`OpenNpcDialog` 不在表中，它在整張表執行完「之後」才開啟對話框，仍留在控制器的互動分派中。

## 關鍵內容（類別 / 函式 / 資料）

- `QuestHookFn = std::function<void(EventBus&, Player&, string_view npcId, SemesterState, SemesterState returnTo)>`：統一鉤子簽章（5 個參數）。
- `struct QuestHook`：登記的鉤子，含 `name`（`string_view`，供測試/記錄）與 `fn`（`QuestHookFn`）。
- `InteractQuestHooks() → const vector<QuestHook>&`：取只建構一次的有序鉤子表（靜態快取）；文件列舉了 9 個已登記鉤子的名稱與順序。
- `RunInteractHooks(EventBus&, Player&, string_view, SemesterState, SemesterState)`：自由函式，依登記順序執行整張表；供測試在不經控制器的情況下直接驅動。

## 相依與在架構中的位置

- **#include（往外）**：`SemesterState.h`（鉤子參數型別）；標準庫 `<functional>`、`<string_view>`、`<vector>`；前向宣告 `Player`、`EventBus`
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（在 E 互動時呼叫 `RunInteractHooks`）、`src/game/controller/InteractDispatch.cpp`（互動分派）、`src/game/quest/QuestHookTable.cpp`（實作體）、`tests/quest/test_quest_hook_table.cpp`（單元測試）
- **繼承 / 實作 / 體現**：realizes [Command / Table（資料化）](../concepts/pat-command.md)
- **每幀管線 / MVC 角色**：Controller 互動管線——在 Spawn→Collision 完成後、`OpenNpcDialog` 前，每次「非閒聊 NPC E 互動」時由 `InteractDispatch` 呼叫 `RunInteractHooks`；位於 MVC 的 Controller 層。

## OO 概念與設計重點

本檔體現了 [Command 模式的「資料化表格」](../concepts/pat-command.md)變體：把可呼叫的任務鉤子（原本硬編碼的函式呼叫序列）包裹成 `QuestHook` 物件並以 `std::function` 儲存在有序表中，使鉤子集合成為資料（可用 `InteractQuestHooks()` 查詢、可在測試中斷言數量與順序），而控制器只需遍歷整張表而無需感知每個鉤子的存在。這也是**開放封閉原則**的體現：新增鉤子只需在 `QuestHookTable.cpp` 追加一行登記，不必修改控制器。`QuestHook::name` 欄位是測試友好的設計，讓測試可以按名稱查詢鉤子而不依賴脆弱的索引順序。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/QuestHookTable.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestHookTable.h) · [← 全檔索引](../files-index.md) · 相關概念：[Command](../concepts/pat-command.md)
