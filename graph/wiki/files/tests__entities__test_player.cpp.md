---
id: "file:tests/entities/test_player.cpp"
type: test
path: tests/entities/test_player.cpp
domain: tests
bucket: entities
loc: 39
classes: []
sources: ["tests/entities/test_player.cpp"]
---
# `test_player.cpp`

> **一句定位**：驗證 `Player` 最基礎的狀態介面——初始 karma（50）、`decreaseKarma` 扣減（karma 可為負）、`resetRainMeter` 歸零，以及 `SetHasUmbrella` 切換持傘旗標。

## 職責

本檔包含 4 個輕量 `TEST_CASE`，是 `Player` 狀態介面的煙霧測試（smoke test）。這些測試涵蓋最基礎的不變式，而更完整的 karma 裁切、金錢、旗標系統與雨量計邊界由 `test_player_core.cpp` 負責。

**初始預設值**：`Player p({0,0})` 後 `GetKarma() == 50`、`HasUmbrella() == false`。

**`decreaseKarma`**：扣 15 後 karma 為 35；再扣 40 後為 -5（確認 karma 可為負，不設下界）。注意：此處測試沒有裁切至 -100 的斷言，與 `test_player_core.cpp` 的 `AddKarma` 裁切 case 形成對比——`decreaseKarma` 與 `AddKarma` 的邊界語意由後者的套件詳細釘住。

**`resetRainMeter`**：初始雨量為 0，`resetRainMeter()` 後仍為 0（呼叫本身冪等，不測有雨量時的效果）。

**`SetHasUmbrella`**：先 false，呼叫 `SetHasUmbrella(true)` 後 true。

## 關鍵內容（類別 / 函式 / 資料）

- `Player::GetKarma()`：業力查詢。
- `Player::HasUmbrella()`：持傘旗標查詢。
- `Player::decreaseKarma(amount)`：扣減業力（不裁切，允許負值）。
- `Player::resetRainMeter()`：雨量計歸零。
- `Player::SetHasUmbrella(bool)`：持傘旗標切換。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/Player.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層煙霧測試）

## OO 概念與設計重點

本檔是 `Player` 的最早期（最簡單）測試套件，功能上是 `test_player_core.cpp` 的前身。兩檔並存：`test_player.cpp` 提供快速的基礎健全性確認，`test_player_core.cpp` 提供完整的邊界條件與複合場景測試。純資料模型的單元測試，無 GL、無 harness、無 EventBus。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_player.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_player.cpp) · [← 全檔索引](../files-index.md)
