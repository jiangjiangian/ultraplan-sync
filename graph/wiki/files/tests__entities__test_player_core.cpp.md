---
id: "file:tests/entities/test_player_core.cpp"
type: test
path: tests/entities/test_player_core.cpp
domain: tests
bucket: entities
loc: 131
classes: []
sources: ["tests/entities/test_player_core.cpp"]
---
# `test_player_core.cpp`

> **一句定位**：驗證 `Player` 純資料狀態的四大支柱——karma `[-100, 100]` 裁切語意、金錢防透支、旗標往返操作，以及雨量計累積（持傘空操作、滿值傳送回正門並發出 `ShowMessage`）。

## 職責

本檔包含 5 個 `TEST_CASE`（其中兩個含多個 SUBCASE），是 `Player` 狀態機最完整的邊界測試套件。

**全新預設值**：karma 50、money 100、rainMeter 0、未持傘、`kFlagHelpedSenior == false`、空字串旗標 == false。

**`AddKarma` 裁切**：4 個 SUBCASE：正向增量（50+30=80）；負向增量 50+(-100)=-50（仍在範圍內）；正向溢出 50+200→100；反覆大量負向 50+(-300)→-100，再 -50 仍 -100（地板）。

**`decreaseKarma` 等價性**：`decreaseKarma(10)` 與 `AddKarma(-10)` 語意等價（以 twin 玩家對照）。

**金錢**：`AddMoney(50)` → 150；`DeductMoney(40)` → 110（成功）；`DeductMoney(200)` 失敗、餘額不變（防透支，回傳 false）。

**旗標往返**：`HasFlag(kFlagHelpedTACh1) == false` → `SetFlag` → `HasFlag == true` → `ClearFlag` → `HasFlag == false`；`ClearFlag("Flag_Never_Set")` 是空操作（不丟例外）。

**`ApplyRain` 雨量計**：3 個 SUBCASE：無傘曝雨 0.5 秒 → `GetRainMeter() ≈ 2.5`（5 u/s × 0.5）；持傘時 `ApplyRain` 為空操作（`rainMeter == 0`）；雨量計 99 曝雨 10 秒超過上限：裁切為 100、傳送回正門（`position ≈ {500, 1860}`）、rainMeter 歸零、發出 ShowMessage `"你淋成落湯雞了…"` 一次。最後的 SUBCASE 訂閱 ShowMessage 並驗證 `hits == 1` 與文字內容。

## 關鍵內容（類別 / 函式 / 資料）

- `Player::AddKarma(delta)`：帶 `[-100, 100]` 裁切的業力修改。
- `Player::decreaseKarma(amount)`：等價於 `AddKarma(-amount)`。
- `Player::AddMoney(amount)`：金錢累加。
- `Player::DeductMoney(amount)` → `bool`：防透支扣款。
- `Player::SetFlag(flagId)`、`HasFlag(flagId)`、`ClearFlag(flagId)`：旗標三元操作。
- `Player::ApplyRain(seconds)`：雨量累積（持傘 no-op；超限傳送 + 發事件）。
- `Player::GetRainMeter()`、`GetPosition()`。
- EventBus（傳送 ShowMessage 時使用）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/Player.h`、`include/game/quest/Flags.h`、`include/engine/events/EventBus.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：驗證 Model 層（`Player` 屬 `World::Model`）的純資料不變式；`ApplyRain` 傳送副作用對應 SurvivalSystem 滿值路徑

## OO 概念與設計重點

本檔展示了「資料模型層的邊界條件測試」最佳實踐：每個 SUBCASE 從全新 `Player` 起步，覆蓋正常、邊界與溢出三種情況。`ApplyRain` 滿值的 SUBCASE 是整合邊界條件（雨量計滿了）與副作用（傳送、ShowMessage）的複合驗證，直接對應 `SurvivalSystem` 的雨量判斷邏輯。以 doctest 的 SUBCASE 而非多個 TEST_CASE 組織，讓相關測試共享設定上下文。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_player_core.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_player_core.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-observer](../concepts/pat-observer.md)
