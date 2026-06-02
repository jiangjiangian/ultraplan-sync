---
id: file:src/game/world/WorldOptions.cpp
type: source
path: src/game/world/WorldOptions.cpp
domain: game
bucket: world
loc: 27
classes: []
sources: ["src/game/world/WorldOptions.cpp"]
---
# `WorldOptions.cpp`

> **一句定位**：從環境變數讀取無障礙選項（`UMBRELLA_REDUCED_MOTION` / `UMBRELLA_LARGE_TARGETS`），是遊戲環境感知與 `World` 建構式之間的唯一橋接點。

## 職責

此檔屬於 game / world 層，只實作一個函式 `ReadWorldOptionsFromEnv()`。它嚴格比對兩個環境變數是否等於字串 `"1"`（任何其他值維持預設 false），分別填入 `WorldOptions.reducedMotion` 和 `WorldOptions.largeTargets`，回傳完整的 `WorldOptions` struct。

設計原則：環境讀取只在程式啟動時由組裝根呼叫一次，`World` 的建構式只接受已解析的 `WorldOptions` 參數，保持 `World` 對環境的純粹性（使用預設 `WorldOptions{}` 的單元測試兩旗標皆 false，不受環境污染）。

## 關鍵內容（類別 / 函式 / 資料）

- `ReadWorldOptionsFromEnv()` — 唯一公開函式；讀取 `UMBRELLA_REDUCED_MOTION` 和 `UMBRELLA_LARGE_TARGETS` 環境變數，回傳 `WorldOptions`。
- `WorldOptions::reducedMotion` — 減少動畫旗標；影響章節字卡淡出、插曲段出口線動畫等視覺效果。
- `WorldOptions::largeTargets` — 擴大互動目標旗標；影響 NPC / 物品的碰撞盒尺寸（無障礙考量）。

## 相依與在架構中的位置

- **#include（往外）**：`WorldOptions.h`（`WorldOptions` struct 宣告）；標準庫 `cstdlib`（`getenv`）/ `cstring`（`strcmp`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `App` 或 `main` 在初始化時呼叫一次，將結果傳入 `World` 建構式。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（初始化時執行，非每幀路徑）；屬 Model 層環境讀取輔助。

## OO 概念與設計重點

嚴格的 `"1"` 比對（非空字串或 truthy 判斷）讓旗標行為完全可預測，避免意外啟用。此函式是「把副作用集中到邊界」原則的體現：只有這一個地方呼叫 `getenv`，其餘程式碼都接收乾淨的 `WorldOptions` 值物件，符合 [DIP](../concepts/arch-dip-renderer.md) 精神。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/WorldOptions.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldOptions.cpp) · [← 全檔索引](../files-index.md)
