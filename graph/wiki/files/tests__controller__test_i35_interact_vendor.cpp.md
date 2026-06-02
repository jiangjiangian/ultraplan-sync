---
id: "file:tests/controller/test_i35_interact_vendor.cpp"
type: test
path: tests/controller/test_i35_interact_vendor.cpp
domain: tests
bucket: controller
loc: 367
classes: [TestInput]
sources: ["tests/controller/test_i35_interact_vendor.cpp"]
---
# `test_i35_interact_vendor.cpp`

> **一句定位**：透過真正的 `GameController::Update()` 迴圈，以腳本化 `TestInput` 端到端驗證兩條主線：互動觸及範圍能讓被牆貼齊的玩家仍能按 E 開對話，以及 Vendor 購買流程正確路由至 `TryBuy` 並更新旗標。

## 職責

本檔是一個「整合層」測試，不走單元替身，而是以相同的 Input 介面驅動完整的 `GameController::Update()` 路徑。包含三個大型 `TEST_CASE`，每個都構造真實的 `World` 與 `GameController`，並以本地定義的 `TestInput` 模擬玩家輸入。

**`TestInput` fixture**：繼承 `InputSource`，實作 `Hold/Release/Tap` 與 `EndFrame()`。`Tap` 讓某鍵恰好在下一個 `Update` 期間為 pressed，`EndFrame` 在 `Update` 後清除 edge 並自動放開 tap 的鍵，完整重現 LiveInput/ScriptInput 的 edge 語意。

**Case 1「走近 Ch1 苦主後按 E 會開啟對話」**：使用 `WalkUpAndTalk` helper，先按住方向鍵走到 NPC 正南方直到位置停住（貼齊移動碰撞箱），再逐格 Tap E 最多 16 次。驗證：`world.Dialog().Active()` 為 true、`NpcId() == "victim"`、玩家在苦主下方（`py >= vy`）且緊鄰（`py <= vy + 40`）。這個 case 釘住了互動觸及範圍（`kInteractReach`）充氣後的幾何保證。

**Case 2「玩家仍無法穿越靜止的 NPC」**：持續按住 W 擠 1200 格，逐格驗證同列時玩家碰撞箱永不嚴格越過 NPC 碰撞箱（`pos.y >= vy`）；最終 `endY >= vy` 確認未穿越。

**Case 3「與 Vendor 互動會路由到 TryBuy（Ch4 醜傘）」**：驅動 FSM 到 `Chapter4_Finals`，傳送到 Vendor 旁，Tap E 開選單、推進到 `AtChoice`，驗證選單有 2 個選項（庫存 + 「先不買，謝謝」），再 Tap E 確認購買。斷言：扣 100 元、設 `kFlagBoughtUglyUmbrella`、`HeldUmbrellaKind() == HeldUmbrella::Ugly`、`ConsumableCount("UglyUmbrella") == 0`（非消耗品）、背包只有 1 列醜傘、`PickupAcquired` 事件一次。

**Case 4「Ch2 主線進度」**：在 `Chapter2_Midterms` 預設三份筆記旗標，向 Vendor 購買 EnergyDrink，先見圖書館管理員（`kFlagMetLibrarian`），再兩次與 bookworm 對話，逐步驗證 `kFlagBookworm` → `kFlagBookwormRecovered` → `kFlagCh2Cleared` 的完整主線進度。

## 關鍵內容（類別 / 函式 / 資料）

- `class TestInput : InputSource`：邊緣語意的腳本化 InputSource，`Hold/Tap/Release/EndFrame`。
- `Frame(controller, in)`：執行一個模擬格（`Update()` + `EndFrame()`）的 helper。
- `FindNpc(world, id)`：在 `world.Objects()` 中依 `NpcId()` 尋找 NPC。
- `FindVendor(world)`：尋找 `IsVendor() == true` 的物件。
- `WalkUpAndTalk(controller, in, world, approach, wantX)`：走近 NPC 後嘗試 16 次 Tap E 開對話。
- `TEST_CASE("走近 Ch1 苦主後按 E 會開啟對話")`：互動觸及範圍幾何鎖定點。
- `TEST_CASE("玩家仍無法穿越靜止的 NPC")`：碰撞不可穿越回歸測試。
- `TEST_CASE("與 Vendor 互動會路由到 TryBuy（Ch4 醜傘）")`：購買流程端到端。
- `TEST_CASE("Ch2 主線進度 — 購買 EnergyDrink、喚醒學霸、Flag_Ch2Cleared")`：Ch2 兩階段進度。

## 相依與在架構中的位置
- **#include（往外）**：`GameController.h`、`World.h`、`Player.h`、`EventBus.h`、`Input.h`、`Key.h`、`Time.h`、`DialogState.h`、`DialogSource.h`、`ChapterVendors.h`、`ItemCatalog.h`、`Chapter2Quest.h`、`SemesterState.h`、`Flags.h`、`GameObject.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 繼承 `InputSource`（`include/engine/input/Input.h`）
- **每幀管線 / MVC 角色**：測試驅動整條 `GameController::Update()` 管線（Survival→Movement→Collision→Spawn→Interact→Sweep），驗證 Controller 層

## OO 概念與設計重點

本檔是 [arch-harness](../concepts/arch-harness.md) 概念的整合測試示範：以相同的 `InputSource` 介面置換真實裝置，走正式產品路徑（不繞過任何系統）但在 headless 環境下執行。`TestInput` 的設計體現了 [Strategy](../concepts/pat-strategy.md) 模式：輸入源可替換，`GameController` 完全不知道自己被測試驅動。每個 case 的資源清理（`Input::SetSource(nullptr)`、`Time::SetFixedStep(0.0f)`、`EventBus::Clear()`）遵循確定性隔離原則。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_i35_interact_vendor.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_i35_interact_vendor.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-harness](../concepts/arch-harness.md) · [Strategy](../concepts/pat-strategy.md)
