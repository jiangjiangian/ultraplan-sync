---
id: file:src/game/controller/screens/EndingScreen.cpp
type: source
path: src/game/controller/screens/EndingScreen.cpp
domain: game
bucket: controller
loc: 37
classes: []
sources: ["src/game/controller/screens/EndingScreen.cpp"]
---
# `EndingScreen.cpp`

> **一句定位**：結局畫面的輸入處理器——偵測玩家在結局選單上的左右移動與確認，並向 `World` 發出 Restart 或 Quit 的應用程式動作請求。

## 職責

`EndingScreen.cpp` 實作自由函式 `HandleEndingMenu(World& world)`，是 Controller 層（`GameController`）每幀呼叫的畫面凍結入口。當目前學期狀態為結局狀態（`IsEndingState(world.Semester().Current())` 回傳 true）時，它接管輸入並回傳 `true`，阻止後續的移動與互動邏輯執行。

函式透過 `Input::IsPressed(Key::Left/Right)` 調動 `world.MoveEndingMenuCursor(±1)` 移動游標，再以 E 或 Enter 確認。確認時根據 `EndingMenuChoiceAt(world.EndingMenuCursor())` 解算 `EndingMenuChoice` 列舉值，分三路：`BackToTitle` 與 `RestartGame` 皆發出 `World::AppAction::Restart`（由 `World` 完整拆解後從標題重建），`Quit` 發出 `World::AppAction::Quit`（唯一真正關閉畫布的路徑）。

若當前狀態不是結局狀態，函式直接回傳 `false`，讓輸入穿透到下一個處理層，維持非結局幀的正常遊玩。

## 關鍵內容（類別 / 函式 / 資料）

- `HandleEndingMenu(World& world) -> bool`：唯一入口；`true` 表示輸入已被結局選單消耗、模擬凍結。
- `IsEndingState(SemesterState)`：引自 `EndingMenuModel.h`，判斷是否為任一 Ending 狀態。
- `EndingMenuChoiceAt(int cursor)`：引自 `EndingMenuModel.h`，游標索引對映 `EndingMenuChoice` 列舉。
- `World::MoveEndingMenuCursor(int delta)`：移動結局選單游標（夾制在 `EndingMenuModel` 規定的範圍）。
- `World::RequestAppAction(World::AppAction)`：向應用程式層請求 Restart 或 Quit。

## 相依與在架構中的位置

- **#include（往外）**：`EndingScreen.h`（宣告 `HandleEndingMenu`）、`World.h`（存取學期狀態與游標）、`EndingMenuModel.h`（`IsEndingState` / `EndingMenuChoiceAt`）、`Input.h` / `Key.h`（輸入查詢）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 的每幀輸入分派直接呼叫，不被其他 .cpp 引入）。
- **繼承 / 實作 / 體現**：—（純自由函式，無繼承）。
- **每幀管線 / MVC 角色**：Controller 層的輸入分派第一道閘；置於 Movement / Interact 邏輯之前，以 `return true` 凍結整條管線。

## OO 概念與設計重點

本檔屬 [MVC](../concepts/arch-mvc.md) 的 **Controller** 層。採自由函式而非物件，符合「無狀態的畫面處理器」慣例——所有狀態由 `World` 持有，函式只讀取並寫回 `World`。`World::RequestAppAction` 是層間溝通的單一出口，避免 Controller 直接操作視窗或畫布。`HandleEndingMenu` 回傳 `bool` 讓呼叫端以短路方式堆疊多個畫面處理器，是 Chain-of-Responsibility 的輕量變體。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/screens/EndingScreen.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/EndingScreen.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
