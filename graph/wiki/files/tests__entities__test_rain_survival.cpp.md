---
id: file:tests/entities/test_rain_survival.cpp
type: test
path: tests/entities/test_rain_survival.cpp
domain: tests
bucket: entities
loc: 290
classes: [TestInput]
sources: ["tests/entities/test_rain_survival.cpp"]
---
# `test_rain_survival.cpp`

> **一句定位**：驗證「淋雨求生」迴圈的三向 tick 邏輯——DrainRain 純恢復、撐傘戶外的慢速累積（ApplyRainSheltered）、無傘戶外的快速致命累積（ApplyRain），以及市集過場完全跳過 tick。

## 職責

此測試是「企劃核心的淋雨求生迴圈」的端到端規格，驗證 `GameController::Update()` 每幀在三種處境下對玩家的雨量計（RainMeter）分別採取哪條路徑。機制如下：

- **排水**（`Player::DrainRain`，-10 u/s）：玩家在建築物內時。
- **慢速累積**（`Player::ApplyRainSheltered`，約 +1.5 u/s）：持傘但在戶外。
- **快速致命累積**（`Player::ApplyRain`，+5 u/s）：戶外且無傘；達到 100 時 RespawnAtGate 把玩家傳送回正門並重設計量器，同時發出含「落湯雞」字樣的 `ShowMessage`。
- **跳過**：在 `Interlude_Market` 章節時完全略過。

兩個測試（`DrainRain` 和 `ApplyRainSheltered`）是純 Player 方法的單元測試，不需要 `GameController`。後兩個測試（致命傳送與三向驗證）透過真正的 `GameController::Update()` 迴圈與 `TestInput` harness 驗證端到端行為。

## 關鍵內容（類別 / 函式 / 資料）

- `TestInput`：實作 `InputSource` 的最小 stub，所有鍵狀態均回傳 false，帶 `EndFrame()` 方法；讓 `GameController` 能在無窗口環境下輪詢輸入。
- `Frame()`：輔助函式，每呼叫一次執行 `controller.Update()` + `in.EndFrame()`。
- `TEST_CASE("淋雨：DrainRain 以 -10 u/s 恢復、裁切於 0，無副作用")`：先以 `ApplyRain(lethal=false)` 灌高 240 幀（約 20 u），再以 `DrainRain` 跑 60 幀（約 -10），斷言嚴格遞減；繼續跑 600 幀斷言裁切於 0，且位置不變（無傳送副作用）。
- `TEST_CASE("ApplyRainSheltered 慢速累積（約 1.5 u/s）且帶致命機制")`：240 幀後斷言約 6（非 20），另一全新 Player 以 `ApplyRain` 跑相同時間到約 20（超過 2.5 倍）；再以另一 Player 開啟致命模式跑 80 秒後斷言已重設、傳送到正門。
- `TEST_CASE("淋雨：戶外無傘玩家累積後觸發致命傳送")`：透過 GameController 跑 1800 幀，訂閱 `ShowMessage` 事件計數「落湯雞」訊息，斷言 `soakMsgHits >= 1`、`maxSeen >= 99`、鋸齒狀下降（`sawHighThenDrop`）。
- `TEST_CASE("撐傘只減緩淋雨（每章皆然）；唯有進入建築才會排乾")`：依序演練三種狀態，最後把玩家座標設在 `kAll[0]`（大勇樓）觸發矩形中心，斷言室內嚴格遞減並最終歸零。
- `TEST_CASE("淋雨：在 Interlude_Market（安全狀態）不累積也不排水")`：強制轉場後跑 600 幀，斷言 `RainMeter == 0`。

## 相依與在架構中的位置

- **#include（往外）**：`game/controller/GameController.h`（真正的受測控制器），`game/world/World.h`，`game/entities/Player.h`，`game/world/Buildings.h`（`kAll` 建築矩形），`game/dialog/DialogState.h`，`game/dialog/DialogSource.h`，`engine/events/EventBus.h`，`game/state/SemesterState.h`，`engine/input/Input.h`，`engine/input/Key.h`，`engine/platform/Time.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`TestInput` 繼承 `InputSource`（`include/engine/input/Input.h`）。
- **每幀管線 / MVC 角色**：直接驗證 Survival 步驟（每幀管線最前端），透過 GameController 整合 Movement → Collision → Sweep 的副作用（RespawnAtGate）。

## OO 概念與設計重點

測試採用 [harness 架構](../concepts/arch-harness.md) 的相同模式：`TestInput` 作為 `InputSource` 注入到 `Input::SetSource()`，令測試可在確定性環境下驅動真正的生產路徑而非 stub。每個測試結束時呼叫 `Input::SetSource(nullptr)` 和 `Time::SetFixedStep(0)` 的 teardown 是 RAII 精神的手動實作，避免測試間互相汙染。

`TEST_CONTENT_DIR` 的 `#error` 保護確保在沒有 CMake 定義的情況下編譯時立即失敗，明確了測試對建置系統的依賴。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_rain_survival.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_rain_survival.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [Observer](../concepts/pat-observer.md)
