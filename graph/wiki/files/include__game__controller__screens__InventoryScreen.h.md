---
id: "file:include/game/controller/screens/InventoryScreen.h"
type: header
path: include/game/controller/screens/InventoryScreen.h
domain: game
bucket: controller
loc: 33
classes: []
sources: ["include/game/controller/screens/InventoryScreen.h"]
---
# `InventoryScreen.h`

> **一句定位**：Tab 背包覆蓋層處理器——開啟期間凍結模擬，提供道具清單的瀏覽與消耗品使用操作。

## 職責

`InventoryScreen.h` 宣告自由函式 `HandleInventory`，是 game controller 層 screens 子目錄的背包覆蓋層 handler。

`HandleInventory(EventBus& bus, World& world)` 以 **Tab 鍵邊緣觸發開關**：開啟時如對話框般完全凍結模擬（回傳 `true`），關閉時回傳 `false`。在 `HandleDialog` 之後檢查，確保對話具有優先權——Tab 無法在對話進行中彈出背包面板。

開啟時的操作：
- **↑/↓**：移動游標選取道具列。
- **←/→**：整頁跳動（`kInventoryRowsPerPage` 頁大小，夾限不環繞）。
- **E/Enter**：在消耗品列按下即使用（套用與拾取相同的效果，呼叫 `ApplyConsumableEffect`，再扣減數量）；在非消耗品列（金幣/雨傘/任務紙張）按下無作用。

EventBus 注入是因為 `ApplyConsumableEffect` 需要 `bus` 發布使用效果訊息（如補充雨量/能量的 ShowMessage）。`bus` 在此以前向宣告引入，毋須拉入完整定義。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `HandleInventory(EventBus& bus, World& world)` → `[[nodiscard]] bool` | 背包覆蓋層幀控制；開啟時回傳 true（凍結）；讀 Tab/↑↓/←→/E/Enter。 |
| `EventBus` | 全域命名空間前向宣告。 |
| `nccu::World` | 前向宣告。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告；無 `#include`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（`Update()` 中在 `HandleDialog` 之後呼叫）、`src/game/controller/screens/InventoryScreen.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輸入處理（第四個畫面凍結 handler，在 EndingMenu/PauseMenu/Dialog 之後）；開啟時世界凍結，僅道具操作有效。

## OO 概念與設計重點

`HandleInventory` 與其他畫面 handler 共享「回傳 `bool` 決定是否凍結」的協議，形成了 **Chain of Responsibility** 風格的幀控制鏈：`GameController::Update()` 依序呼叫各 handler，第一個回傳 `true` 的即持有該幀，後續 handler 和 ISystem 管線均不執行。

「在消耗品列有效/非消耗品列無效」的 E/Enter 語意，要求背包檢視知道每個道具的型別——這依賴 ISP 角色系統（`IInteractable`/`IMortal` 等），使背包只調用每個道具「自己知道」的使用方式，Controller 不需 switch 分流。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/screens/InventoryScreen.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/InventoryScreen.h) · [← 全檔索引](../files-index.md)
