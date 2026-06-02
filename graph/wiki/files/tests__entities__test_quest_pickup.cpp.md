---
id: file:tests/entities/test_quest_pickup.cpp
type: test
path: tests/entities/test_quest_pickup.cpp
domain: tests
bucket: entities
loc: 28
classes: []
sources: ["tests/entities/test_quest_pickup.cpp"]
---
# `test_quest_pickup.cpp`

> **一句定位**：驗證 `QuestFlagPickup`（任務旗標拾取物）的核心契約——撿取後在玩家身上設旗標並自我停用，以及對 `nullptr` 的安全空操作。

## 職責

這個測試檔案針對 `QuestFlagPickup` 的 `Interact()` 方法驗證兩條主要不變式，屬於 `tests/entities` 層的純邏輯單元測試，無需 GL context 即可執行。

第一條不變式：當玩家呼叫 `item.Interact(&p)` 時，以 `nccu::kFlagFoundForm` 為旗標名稱建構的 `QuestFlagPickup` 必須把旗標寫入玩家（`p.HasFlag(nccu::kFlagFoundForm)` 變 true），同時讓物件本身失活（`item.IsActive()` 變 false）。測試事前先確認兩個條件均未成立，確保斷言的方向正確。

第二條不變式：互動對象為 `nullptr` 時，`Interact(nullptr)` 不得崩潰，且物件狀態維持 `IsActive() == true`——即安全的空操作。這防禦 Controller 在掃描物件時因指標無效而觸發 UB。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("QuestFlagPickup 在玩家身上設旗標並自我停用")`：以旗標 `nccu::kFlagFoundForm` 建構撿取物，呼叫 `Interact(&p)` 後斷言 `p.HasFlag` 與 `!item.IsActive()`。
- `TEST_CASE("QuestFlagPickup 互動對象為 null 時是安全的空操作")`：以任意旗標字串 `"Flag_X"` 建構，呼叫 `Interact(nullptr)` 後斷言物件仍啟用、不崩潰。
- `nccu::kFlagFoundForm`：Ch1 申請書任務旗標常數，由 `game/quest/Flags.h` 提供。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Vec2.h`（座標），`game/entities/Player.h`（互動對象），`game/entities/QuestFlagPickup.h`（受測類別），`game/quest/Flags.h`（旗標常數）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料邏輯單元測試，不在管線內）

## OO 概念與設計重點

這個測試是典型的 doctest 單元測試，重點在驗證 `QuestFlagPickup` 實作的 `IInteractable` 介面契約。撿取物失活後不再被 `ForEachRole<IInteractable>` 走訪（`isActive_=false` 的 mark-then-sweep 語意），本測試從外部確認這個「互動即失活」的不變式。`nullptr` 的防禦測試體現了對 ISP 角色查詢可能回傳 null 的同一精神。

## 連結

[🕸 圖譜節點](../../index.html#node=file:tests/entities/test_quest_pickup.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_quest_pickup.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[ISP Roles](../concepts/oo-isp-roles.md)
