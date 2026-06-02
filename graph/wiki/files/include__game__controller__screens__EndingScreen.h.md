---
id: "file:include/game/controller/screens/EndingScreen.h"
type: header
path: include/game/controller/screens/EndingScreen.h
domain: game
bucket: controller
loc: 30
classes: []
sources: ["include/game/controller/screens/EndingScreen.h"]
---
# `EndingScreen.h`

> **一句定位**：結局畫面處理器——到達 `Ending_*` 狀態後完全凍結世界，只開放底部三選項選單（回首頁/重新開始/結束）。

## 職責

`EndingScreen.h` 宣告自由函式 `HandleEndingMenu`，是 game controller 層 screens 子目錄中優先級最高的畫面凍結 handler。

`HandleEndingMenu(World& world)` 在當前學期狀態為 `Ending_*`（任何結局狀態）時回傳 `true`，凍結整個世界，並處理底部三選項選單的輸入：
- **←/→**：移動選單游標（在三個選項間移動）。
- **E/Enter**：確認反白選項，以 `World::RequestAppAction` 傳送意圖：回首頁/重新開始皆請求 `Restart`（`main.cpp` 把整局拆回標題畫面）；結束請求 `Quit`（唯一關閉視窗的路徑）。
- **ESC**：不讀——ESC 是 `main.cpp` 所擁有的直接離開鍵，由 `main.cpp` 的外層迴圈處理。

`HandleEndingMenu` 是整個 Controller 管線中第一個被檢查的 handler（在 `HandlePauseMenu`/`HandleDialog`/`HandleInventory` 之前），確保結局狀態下玩家只能操作選單，其他所有輸入路徑均不可達。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `HandleEndingMenu(World& world)` → `[[nodiscard]] bool` | 處於 `Ending_*` 時回傳 `true`（凍結）；讀 ←/→/E/Enter，以 `RequestAppAction` 傳送 Restart 或 Quit。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告 `nccu::World`；無 `#include`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（`Update()` 最先呼叫的 handler）、`src/game/controller/screens/EndingScreen.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輸入處理（最高優先級的畫面凍結 handler）；在所有 ISystem 管線、互動派發、建築進入之前執行，結局狀態下整幀被此 handler 持有。

## OO 概念與設計重點

`HandleEndingMenu` 是結局流程的**終端狀態閘門**：一旦進入結局，遊戲世界的所有模擬都凍結，玩家只有三個明確的出口（回首頁/重開/結束）。這體現了 [State 模式（pat-state）](../concepts/pat-state.md) 中「每個狀態控制自己的行為邊界」的思想——雖然此處以輸入 handler 而非狀態子類別實現，效果相同。

`World::RequestAppAction` 的「記錄意圖而非直接執行」設計（真正的拆解發生在 `main.cpp` 外層迴圈）維持了 MVC 分層：Controller 只改 Model（`World` 上的意圖旗標），View 和 `main.cpp` 的組裝層負責響應意圖並執行真正的場景切換。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/screens/EndingScreen.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/EndingScreen.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
