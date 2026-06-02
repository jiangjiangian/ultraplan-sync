---
id: "file:tests/vendor/test_vendor_decline.cpp"
type: test
path: tests/vendor/test_vendor_decline.cpp
domain: tests
bucket: vendor
loc: 158
classes: [TestInput]
sources: ["tests/vendor/test_vendor_decline.cpp"]
---
# `test_vendor_decline.cpp`

> **一句定位**：透過 GameController 真實輸入迴圈，端到端驗證向 Vendor 購買「絕不強迫」——選擇「不買」後一切狀態不變，且攤位之後仍可再次購買。

## 職責

本測試用 `TestInput` 驅動真實的 `GameController::Update()`，走過完整的購買對話流程。場景設定為 Ch4 集英樓醜傘攤位（售價 100 元，購買設定 `kFlagBoughtUglyUmbrella`）。

**測試流程**：
1. 設定 Ch4 狀態，找到 Vendor，玩家移到攤主附近（距離 8px）。
2. 按 E 開啟購買選單；前進到選擇畫面（最多 16 幀）。
3. 斷言選單有 2 個選項，最後一個標籤為「先不買，謝謝」，無 `setsFlag`，`karmaDelta == 0`。
4. 游標移到最後一項（「不買」）並按 E 確認。
5. 驗證對話關閉；money/karma/旗標/背包/事件計數皆與確認前完全相同（購買未被強迫）。
6. **二次購買驗證**：重新接近攤主，再次打開選單並確認購買第 0 個選項（醜傘）——錢包扣款 100 元、旗標設定、`pickupHits == 1`，確認「不買」動作沒有弄壞攤位。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：最小 `InputSource`，支援 `Hold/Release/Tap/EndFrame`。
- `Frame(GameController&, TestInput&)`：步進函式。
- `FindVendor(World&)`：遍歷 `World::Objects()` 找第一個 `IsActive() && IsVendor()` 的物件。
- `World::Dialog().AtChoice()` / `World::Dialog().Choices()` / `World::Dialog().ChoiceCursor()` — 被觀察的對話狀態。
- `nccu::kFlagBoughtUglyUmbrella` — 驗證購買後才設定。
- `EventBus::Instance().Subscribe(EventType::PickupAcquired, ...)` — 計數 `pickupHits`。

## 相依與在架構中的位置

- **#include（往外）**：`game/controller/GameController.h`、`game/world/World.h`、`game/entities/Player.h`、`game/dialog/DialogState.h`、`game/dialog/DialogSource.h`、`game/quest/ChapterVendors.h`、`game/quest/Flags.h`、`engine/input/Input.h`（TestInput 基底）、`engine/platform/Time.h`、`engine/events/EventBus.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 實作 `include/engine/input/Input.h`。
- **每幀管線 / MVC 角色**：Controller + Model 整合測試，覆蓋 Interact → Dialog → Choice → 不買 的完整路徑。

## OO 概念與設計重點

以 [Harness](../concepts/arch-harness.md) 的 TestInput 替換真實輸入，在不開窗口的情況下走過真實的購買互動路徑。「不買後攤位仍可使用」的斷言防止單次測試觸發的狀態殘留破壞之後的互動。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/vendor/test_vendor_decline.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/vendor/test_vendor_decline.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
