---
id: "file:include/game/controller/screens/PauseScreen.h"
type: header
path: include/game/controller/screens/PauseScreen.h
domain: game
bucket: controller
loc: 34
classes: []
sources: ["include/game/controller/screens/PauseScreen.h"]
---
# `PauseScreen.h`

> **一句定位**：遊戲內暫停選單處理器——M 開關，開啟時凍結世界，可疊加說明覆蓋層（兩頁，←/→ 翻動）。

## 職責

`PauseScreen.h` 宣告自由函式 `HandlePauseMenu`，是 game controller 層 screens 子目錄的暫停選單 handler，優先級僅次於 `HandleEndingMenu`。

`HandlePauseMenu(World& world)` 回傳：M 開啟選單的那一幀、或選單/說明覆蓋層已開啟時回傳 `true`（凍結）；與選單無關時回傳 `false`。使用 M 鍵切換（開啟/「繼續」關閉）；ESC 保留給 `main.cpp` 的直接離開路徑，此 handler 不讀 ESC。

說明覆蓋層疊在暫停選單之上：其開啟時 M/E/Enter 將它收回選單，覆蓋層不完全關閉暫停選單。覆蓋層分兩頁（←/→ 翻動）：
- 頁 0：操作說明 + 遊玩目標
- 頁 1：雨傘外觀介紹 + 道具須知 + 結局說明

選單項目（繼續/說明/回首頁/結束）以 ↑/↓ 移動游標、E/Enter 確認。「繼續」關閉選單恢復遊戲；「回首頁」和「結束」透過 `World::RequestAppAction` 記錄意圖（Restart/Quit），真正的拆解發生在 `main.cpp` 外層迴圈。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `HandlePauseMenu(World& world)` → `[[nodiscard]] bool` | 暫停選單幀控制；M 開關；含說明覆蓋層（兩頁翻動）。 |
| `nccu::World` | 前向宣告。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告 `nccu::World`；無 `#include`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（`Update()` 中在 `HandleEndingMenu` 之後、`HandleDialog` 之前呼叫）、`src/game/controller/screens/PauseScreen.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輸入處理（第二優先的畫面凍結 handler）；開啟時凍結模擬，玩家只能操作選單或瀏覽說明。

## OO 概念與設計重點

`HandlePauseMenu` 的「說明覆蓋層疊在暫停選單之上」的設計是一種輕量版的**Composite/Layer**模式：暫停選單和說明覆蓋層共享同一個 handler，以 World 的 `helpOpen_` 旗標和 `helpPage_` 游標追蹤覆蓋層狀態，兩者同屬「世界凍結」的幀，無需獨立的 handler 層級。

`World::RequestAppAction`（記錄意圖）vs `main.cpp`（執行意圖）的分層，與 `HandleEndingMenu` 相同，維持 MVC 架構清晰：Controller 只寫 Model，組裝層（`main.cpp`）負責響應。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/screens/PauseScreen.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/PauseScreen.h) · [← 全檔索引](../files-index.md)
