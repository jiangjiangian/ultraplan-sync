---
id: file:src/game/controller/screens/PauseScreen.cpp
type: source
path: src/game/controller/screens/PauseScreen.cpp
domain: game
bucket: controller
loc: 85
classes: []
sources: ["src/game/controller/screens/PauseScreen.cpp"]
---
# `PauseScreen.cpp`

> **一句定位**：暫停選單與說明疊層的輸入處理器——M 鍵開關、方向鍵選項、Enter 確認，以及無障礙旗標的就地翻轉，開啟期間凍結模擬。

## 職責

`PauseScreen.cpp` 實作自由函式 `HandlePauseMenu(World& world)`，是 Controller 層最外層的輸入凍結器，優先於背包與移動處理。函式在每幀最前端處理暫停選單，以三層狀態（未開啟 / 選單 / 說明疊層）構成巢狀邏輯。

**說明疊層優先**：若 `world.HelpOpen()` 為 true，Left/Right 切換說明頁（利用 `kGameHelpPageCount` 計算繞回），M / E / Enter 關閉疊層並回到選單；此層直接 `return true`，絕不讓按鍵同時移動選單游標。

**選單模式**：Up/Down 呼叫 `world.MoveMenuCursor(±1)` 移動游標。M 鍵作為「快速繼續」直接關閉選單。Enter 按游標索引分支：
- 0（繼續）：`SetMenuOpen(false)`
- 1（說明）：`SetHelpOpen(true)` 開啟疊層
- 2（減少動畫）：`SetReducedMotion(!world.ReducedMotion())` 就地翻轉無障礙旗標
- 3（擴大目標）：`SetLargeTargets(!world.LargeTargets())` 就地翻轉另一個無障礙旗標
- 4（重新開始）：`world.RequestAppAction(Restart)`
- default（離開）：`world.RequestAppAction(Quit)`

無障礙旗標翻轉後選單不關閉，使玩家能在同一列即時看到 `[開]/[關]` 的狀態更新。`LargeTargets` 的新值在下一遊玩幀立即被 E 互動探測距離的讀取端（`World::LargeTargets()`）感知。

**遊玩模式穿透**：選單未開啟時，M 鍵開啟後回傳 `true`；其他鍵回傳 `false` 讓輸入穿透。

## 關鍵內容（類別 / 函式 / 資料）

- `HandlePauseMenu(World& world) -> bool`：唯一入口；`true` 凍結模擬。
- `nccu::kGameHelpPageCount`：引自 `GameHelpPages.h`，說明疊層的頁數上限。
- `World::SetHelpOpen(bool)` / `World::HelpOpen()` / `World::HelpPage()` / `World::SetHelpPage(int)`：說明疊層狀態。
- `World::SetMenuOpen(bool)` / `World::MenuOpen()` / `World::MoveMenuCursor(int)` / `World::MenuCursor()`：選單狀態。
- `World::SetReducedMotion(bool)` / `World::SetLargeTargets(bool)`：無障礙旗標。
- `World::RequestAppAction(World::AppAction)`：應用程式層請求。

## 相依與在架構中的位置

- **#include（往外）**：`PauseScreen.h`、`World.h`（所有選單狀態與請求）、`GameHelpPages.h`（`kGameHelpPageCount`）、`Input.h` / `Key.h`。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 每幀最前端呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：Controller 層輸入分派的最前端凍結器，優先於背包、移動、互動的所有邏輯。

## OO 概念與設計重點

本檔屬 [MVC](../concepts/arch-mvc.md) 的 **Controller** 層。三層巢狀（遊玩 / 選單 / 說明）以「最後處理者先回傳」的早回傳策略實作，避免按鍵在多層間意外穿透（說明疊層的 M 鍵絕不同時移動選單游標，是其中最重要的邊界條件）。無障礙旗標的就地翻轉（不關選單）是 UX 細節，確保玩家能立即讀到 [開]/[關] 回饋。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/screens/PauseScreen.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/PauseScreen.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
