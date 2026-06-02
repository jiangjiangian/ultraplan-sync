---
id: file:include/game/world/WorldOptions.h
type: header
path: include/game/world/WorldOptions.h
domain: game
bucket: world
loc: 39
classes: [WorldOptions]
sources: ["include/game/world/WorldOptions.h"]
---
# `WorldOptions.h`

> **一句定位**：注入 `World` 建構子的無障礙選項 DTO（`reducedMotion`／`largeTargets`）及從環境變數解析它的工廠函式，把環境讀取移出 World 讓測試取得確定性行為。

## 職責

`WorldOptions` 是一個僅含兩個 `bool` 成員的純資料 struct，作為 World 建構子的參數注入「減少動畫」與「擴大目標」兩個無障礙偏好。設計動機在注解中說得清楚：若 World 建構子自行呼叫 `std::getenv`，則單元測試就必須設定或清除環境變數才能取得確定性行為。把環境讀取移到 `ReadWorldOptionsFromEnv()` 工廠函式中，並讓其只在 `main.cpp`（組裝根）被呼叫一次，測試則直接建構 `WorldOptions{}`（兩旗標預設 `false`），與環境完全解耦。

兩個選項的效果：
- `reducedMotion`：開啟時 View 抑制章節卡片滑動、過場出口標記掃動與提示淡出；結局卡片首幀即全不透明。
- `largeTargets`：開啟時 E 互動探測範圍由每側 8 px 增為 16 px（對話判定框由 40×40 增為 56×56），使手部顫抖的玩家不必像素級對齊。移動碰撞體刻意不受影響，僅對話可達範圍變大。

`ReadWorldOptionsFromEnv()` 以 `[[nodiscard]]` 標記，確保呼叫端不意外丟棄回傳值；對應的環境變數值為 "1" 才開啟，未設或 "0" 維持預設 `false`。

## 關鍵內容（類別 / 函式 / 資料）

- `WorldOptions`（struct）：
  - `reducedMotion`（`bool`，預設 `false`）：減少動畫偏好。
  - `largeTargets`（`bool`，預設 `false`）：擴大目標偏好。
- `ReadWorldOptionsFromEnv() -> WorldOptions`（`[[nodiscard]]`）：從行程環境變數解析 `WorldOptions`；實作於 `src/game/world/WorldOptions.cpp`；供 `main.cpp` 組裝根使用。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 include）。
- **被誰使用（往內）**：`include/game/world/World.h`（作為建構子參數 `WorldOptions opts = {}`）；`src/app/scenes/GameplayScene.cpp`（傳入解析好的選項建構 World）；`src/game/world/WorldOptions.cpp`（`ReadWorldOptionsFromEnv` 實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：啟動時一次性注入，不參與每幀管線。注入後的值以 `World::ReducedMotion()` / `LargeTargets()` 等 getter 暴露給 View 與 Controller 讀取。

## OO 概念與設計重點

`WorldOptions` 是 **依賴注入（Dependency Injection）** 的輕量應用：把「不純」的外部環境讀取（`std::getenv`）封裝在一個明確的工廠函式中，只在組裝根呼叫一次，其結果作為純資料傳入 `World`。這使 `World` 的建構子保持純粹（只接受值，不做副作用查詢），也讓測試構造 `WorldOptions{}` 即可獲得完全確定性的無障礙關閉狀態。`[[nodiscard]]` 標記確保解析結果不被意外丟棄，是防禦性 API 設計的體現。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/WorldOptions.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/WorldOptions.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
