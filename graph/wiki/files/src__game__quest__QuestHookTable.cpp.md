---
id: file:src/game/quest/QuestHookTable.cpp
type: source
path: src/game/quest/QuestHookTable.cpp
domain: game
bucket: quest
loc: 65
classes: []
sources: ["src/game/quest/QuestHookTable.cpp"]
---
# `QuestHookTable.cpp`

> **一句定位**：把四章的 E 互動任務自由函式組裝成一張有序 hook 表，並提供 `RunInteractHooks` 作為 `GameController` 每幀 NPC 互動的單一分派入口。

## 職責

此檔屬於 game / quest 層，是遊戲每幀互動管線的「路由表組裝器」。它只做兩件事：

第一，`InteractQuestHooks()` 以函式區域 static 的方式一次性建構一個 `std::vector<QuestHook>` 常數表，依嚴格排定的順序把第一至四章的各自由函式包進轉接 lambda：`TryReturnVictimUmbrella`、`TryReturnTaForm`、`TryRescueBookworm`、`TryMeetLibrarian`、`TryLendLibrarianUmbrella`、`TryReturnLibrarianUmbrella`、`TryApplyCh2Ripple`、`TryAdvanceCh3Trade`、`TryApplyCh3Ripple`、`TryApplyCh4Ripple`，共 10 個 hook。此順序具關鍵語意性：後面的 hook 可能讀取前面 hook 所設下的旗標（例如 `TryMeetLibrarian` 設下的 `Flag_MetLibrarian` 閘控後續圖書館員劇情），因此絕不可重排。

第二，`RunInteractHooks` 線性逐一呼叫這張表，把 `(EventBus&, Player&, npcId, state, returnTo)` 透傳給每個 hook。這讓 `GameController` 不需知道任何具體章節函式，只持有一個對 `RunInteractHooks` 的單一呼叫。

所有 lambda 的簽章統一為 `(EventBus& bus, Player& p, string_view id, SemesterState s, SemesterState ret)`，不發布事件的 hook 以 `(EventBus&)` 忽略 bus 參數；發布事件者則向下傳遞，使表格整體同質。

## 關鍵內容（類別 / 函式 / 資料）

- `InteractQuestHooks()` — 函式區域 static，回傳常數 `const std::vector<QuestHook>&`，包含 10 個有序 hook 的轉接 lambda。
- `RunInteractHooks(EventBus&, Player&, string_view npcId, SemesterState, SemesterState)` — 公開函式；線性執行整張 hook 表，為 `GameController` 的唯一互動分派入口。
- 各 hook 的命名 `name` 欄位（字串字面值）：`"TryReturnVictimUmbrella"` 至 `"TryApplyCh4Ripple"`，僅用於除錯識別。

## 相依與在架構中的位置

- **#include（往外）**：`QuestHookTable.h`（`QuestHook` 結構與 `RunInteractHooks` 宣告）、`Chapter1Quest.h` / `Chapter2Quest.h` / `Chapter3Quest.h` / `Chapter4Quest.h`（實際被包裝的各章節自由函式）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；主要被 `GameController` 在每幀 `Update` 的 E 互動步驟呼叫 `RunInteractHooks`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層的互動分派環節（每幀管線中 RunInteractHooks 步驟）。`GameController` 在偵測到 E 鍵互動後呼叫 `RunInteractHooks`，後者轉呼叫整張表；各 hook 函式修改 `Player` 狀態並可透過 `EventBus` 發布事件。

## OO 概念與設計重點

此檔體現了「Command 表」（[Command](../concepts/pat-command.md)）與「Strategy 組合」（[Strategy](../concepts/pat-strategy.md)）的精髓：把各章節的互動邏輯包成同質的可呼叫物，儲存進一張有序表，讓呼叫端對實作一無所知。函式區域 static 的惰性初始化手法與 `EventBus` 的 [Singleton](../concepts/pat-singleton.md) 單例一致，確保表格只建構一次且執行緒安全（C++11 magic static）。順序固定的不變式透過程式碼註解明確記載，是本設計的最重要邊界條件。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/QuestHookTable.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/QuestHookTable.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Command](../concepts/pat-command.md) · [Strategy](../concepts/pat-strategy.md) · [Singleton](../concepts/pat-singleton.md)
