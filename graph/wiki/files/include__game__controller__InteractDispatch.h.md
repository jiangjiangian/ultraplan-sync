---
id: "file:include/game/controller/InteractDispatch.h"
type: header
path: include/game/controller/InteractDispatch.h
domain: game
bucket: controller
loc: 37
classes: []
sources: ["include/game/controller/InteractDispatch.h"]
---
# `InteractDispatch.h`

> **一句定位**：每幀 E 鍵互動的派發進入點——整合對話、拾取、商店開啟三條路徑，並透過 QuestHook 表取代原本 14 個內嵌的 `TryXxx` 呼叫。

## 職責

`InteractDispatch.h` 宣告自由函式 `DispatchInteract`，負責每幀 E 鍵的互動派發，屬 game controller 層的輸入處理部分。由於此函式讀取 E 鍵邊緣與玩家的觸及盒，它「留在 Controller 層而非 ISystem」，因為 ISystem 只能操作 Model，不讀輸入。

`DispatchInteract` 的核心邏輯（定義在 `InteractDispatch.cpp`）：
- **非閒談型 NPC**：依序跑過已註冊的 QuestHook 表（`RunInteractHooks`），再依 `(npcId, state)` 開啟對應對話。QuestHook 表取代了原本約 14 個內嵌的 `TryXxx` 呼叫，把觸發條件的資料化在 `quest/QuestHookTable.cpp`，順序與自我守門語意不變。
- **閒談型 NPC**：短路到自身的逐行循環 `Interact()`。
- **非 NPC（拾取物/Vendor）**：走 `IInteractable` 角色介面的 `Interact()`。

`pendingVendor` 以非擁有觀察指標的形式（`Vendor*&` 傳引用）在 E 開啟商店時被設為對應攤主，讓 `HandleDialog` 在較晚一幀的確認分支時能讀取它以導向 `Vendor::TryBuy`。攤主選單放棄/購買/非攤主對話開啟時清空。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `DispatchInteract(EventBus& bus, World& world, Vendor*& pendingVendor)` | 自由函式；讀 E 鍵邊緣與玩家觸及盒，派發互動；設定或清空 `pendingVendor`。 |
| `Vendor` | 全域命名空間前向宣告（商店 NPC）。 |
| `EventBus` | 全域命名空間前向宣告（事件匯流排，毋須拉入完整定義）。 |
| `nccu::World` | 前向宣告（World 所在命名空間）。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告，無 `#include`；最小化相依。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（每幀呼叫 `DispatchInteract`，傳入 `pendingVendor_` 成員）、`src/game/controller/InteractDispatch.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輸入處理；在每幀管線的 ISystem 執行「之後」、幀末清除「之前」呼叫（對應 `GameController::DispatchInteract()` 的位置），屬於「E 互動 `RunInteractHooks`」步驟。

## OO 概念與設計重點

把 `DispatchInteract` 從 `GameController::Update()` 中抽出為獨立自由函式，是 **SRP** 的應用：互動派發邏輯有自己的測試場景（`test_i35_interact_vendor`、`test_i6_interact_reach`）可以直接呼叫函式，而不需啟動完整的更新管線。

QuestHook 表的設計（以 `RunInteractHooks` 走資料表取代 14 個 `TryXxx` 呼叫）體現了**開放/封閉原則（OCP）**：新增或修改任務觸發條件只需修改 `QuestHookTable.cpp` 的資料，`DispatchInteract` 本身不需改動。`pendingVendor` 以「非擁有觀察指標＋參考傳遞」的設計，使跨幀的購買狀態不需引入額外的共享所有權（如 `shared_ptr`），維持了清晰的生命週期語意。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/InteractDispatch.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/InteractDispatch.h) · [← 全檔索引](../files-index.md)
