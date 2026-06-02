---
id: file:tests/quest/test_economy_loop.cpp
type: test
path: tests/quest/test_economy_loop.cpp
domain: tests
bucket: quest
loc: 90
classes: []
sources: ["tests/quest/test_economy_loop.cpp"]
---
# `test_economy_loop.cpp`

> **一句定位**：驗證迴圈經濟的三個不變量：`Player::AddMoney` 在 300 軟上限封頂（負數不受限）、`ClearConsumables` 清空背包，以及各章 `ChapterPickups` 金額表與 World 中確實生成 5 枚 Ch1 金幣。

## 職責

此測試檔覆蓋遊戲的經濟子系統，四個 TEST_CASE 各自鎖定一個關鍵不變量：

1. **`AddMoney` 軟上限**：初始 100，加 150 → 250，加 100 → 封頂 300（`Player::kMoneySoftCap`），再加 50 無效；但 `AddMoney(-120)` 仍可扣到 180，`DeductMoney(80)` 同樣可扣。確認上限是「天花板」而非雙側限制。

2. **`ClearConsumables`**：先 `AddConsumable("HotPack")` 兩次、`"EnergyDrink"` 一次，呼叫後全部歸零，`ConsumeOne` 也返回 false。

3. **各章金幣表**：Ch1 5 枚、總值 50；Ch2 3 枚、總值 40（且 `> 35`——防卡關不變量，確保身無分文進 Ch2 的玩家能買到 35 元的 EnergyDrink）；幕間/Ch3/Ch4 皆為空。

4. **World 生成確認**：`World("", false)` 後用 `dynamic_cast<CashPickup*>` 統計確認 5 枚、總值 50，驗證迴圈資料表確實接上世界生成管線。

## 關鍵內容（類別 / 函式 / 資料）

- `Player::kMoneySoftCap`（== 300）：軟上限常數。
- `Player::AddMoney`、`DeductMoney`、`AddConsumable`、`ClearConsumables`、`ConsumeOne`。
- `nccu::ChapterPickups(SemesterState)`：回傳對應章節的 `CashPickup` 資料清單（含 `pos` 與 `value`）。
- `TEST_CASE("Player::AddMoney 只在 300 軟上限封頂，從不設下限")`。
- `TEST_CASE("Player::ClearConsumables 清空整個消耗品背包")`。
- `TEST_CASE("ChapterPickups：Ch1 約 50、Ch2 防卡關 >35，其餘為空")`：包含 `ch2total > 35` 的防卡關守門。
- `TEST_CASE("World 確實生成 Ch1 的 CashPickup（迴圈已接上）")`。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterPickups.h`、`CashPickup.h`、`Player.h`、`World.h`、`SemesterState.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層（`Player` 金錢/消耗品、`World` 生成）的經濟邏輯。

## OO 概念與設計重點

「Ch2 總值 > 35」是一個「防止卡關的業務規則測試」——它不只驗證當前值（40）正確，更表達了「此值永遠必須高於 35」的不變量。這種寫法讓任何壓縮 Ch2 金幣的重構都立即失敗，是比直接斷言 `== 40` 更強的保護。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_economy_loop.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_economy_loop.cpp) · [← 全檔索引](../files-index.md)
