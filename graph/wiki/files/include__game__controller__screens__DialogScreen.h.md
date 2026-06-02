---
id: "file:include/game/controller/screens/DialogScreen.h"
type: header
path: include/game/controller/screens/DialogScreen.h
domain: game
bucket: controller
loc: 41
classes: []
sources: ["include/game/controller/screens/DialogScreen.h"]
---
# `DialogScreen.h`

> **一句定位**：對話畫面處理器——對話開啟期間凍結世界，推進對話、套用選項副作用、把確認的攤主庫存選項導向 `Vendor::TryBuy`，屬 Controller 輸入層。

## 職責

`DialogScreen.h` 宣告自由函式 `HandleDialog`，是遊戲最複雜的畫面處理器，負責對話進行期間的完整幀控制。屬 game controller 層 screens 子目錄。

`HandleDialog` 回傳 `bool`：對話開啟時回傳 `true`（整個世界被凍結，`GameController::Update()` 在呼叫此函式後若得到 `true` 則略過 ISystem 管線、互動派發、建築進入偵測和清除）；無對話進行時回傳 `false`。

核心職責：
1. **推進對話**：接收 `InputHandler` 的 `TickDialogAdvance` 結果，推進 `DialogState`（翻頁→下一行→進入選單模式→確認選項）。
2. **套用選項副作用**：確認選項後呼叫 `ApplyDialogChoice(player, *choice)` 套用業力/旗標。
3. **攤主購買導向**：若 `dlg.NpcId() == kVendorContext` 且確認索引 != 庫存數（非放棄），呼叫 `pendingVendor->TryBuy(index)`；若確認為放棄索引，清空 `pendingVendor` 且不購買。
4. **名冊結算**：對話確認後若觸發章節轉場，於本幀結尾呼叫 `sceneRouter.SettleRoster(world)`。

`pendingVendor` 以參考傳入，因為本函式需要在攤主放棄/購買/非攤主對話開啟時清空它。`input` 以參考傳入供 `TickDialogAdvance` 使用並在對話未開啟時呼叫 `ResetDialogAdvance`。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `HandleDialog(EventBus& bus, World& world, Vendor*& pendingVendor, InputHandler& input, SceneRouter& sceneRouter)` → `bool` | 對話幀控制；凍結時回傳 true；5 個參數各有特定職責。 |
| `[[nodiscard]]` 屬性 | 呼叫端必須使用回傳值（忽略會被編譯器警告）。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告（`Vendor`、`EventBus`、`nccu::World`、`nccu::InputHandler`、`nccu::SceneRouter`）；無 `#include`，最小化標頭相依。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（在 `Update()` 中呼叫，凍結時早期回傳）、`src/game/controller/screens/DialogScreen.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輸入處理（第三個畫面凍結 handler，優先於 ISystem 管線）；在 `HandleEndingMenu`/`HandlePauseMenu` 之後、ISystem 管線之前檢查。

## OO 概念與設計重點

`HandleDialog` 採**Guard Clause**（早期回傳）設計：對話開啟時立即回傳 `true`，`GameController::Update()` 略過後續所有模擬步驟，形成清晰的幀凍結語意，不需深層巢狀 if。

`[[nodiscard]]` 屬性是 C++17 的編譯期安全網，確保呼叫端不會意外忽略「此幀是否已被對話持有」的回傳值，防止因忽略回傳值而繼續跑 ISystem 管線的錯誤。

攤主購買確認的流程（`kVendorContext` 哨兵 → 索引比對 → `TryBuy`）是跨幀狀態（`pendingVendor_`）的消費端，與 `InteractDispatch`（生產端）形成清晰的「設定/消費」配對，而不需引入事件或全局狀態。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/screens/DialogScreen.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/screens/DialogScreen.h) · [← 全檔索引](../files-index.md)
