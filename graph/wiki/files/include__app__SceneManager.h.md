---
id: file:include/app/SceneManager.h
type: header
path: include/app/SceneManager.h
domain: app
bucket: 
loc: 91
classes: [SceneManager]
sources: ["include/app/SceneManager.h"]
---
# `SceneManager.h`

> **一句定位**：持有場景堆疊並驅動全程式唯一應用主迴圈的 `SceneManager`，是 app 層的「狀態機核心」。

## 職責

`SceneManager` 以 `std::vector<std::unique_ptr<IScene>>` 作為場景堆疊，每幀只有頂端場景為現役，其餘場景暫停（保留狀態但不更新）。`Push / Replace / Pop / Restart / Quit` 等場景切換操作一律在「該幀 Draw 之後」才套用（`ApplyCommand`），嚴格保障幀中途不改變現役場景的不變式。

`Run(Window&, IRenderer&, Harness&)` 整合了每幀的全部流程：視窗輪詢（WindowShouldClose）、`DrawScope` 開關、`Harness::BeginFrame / EndFrame`（自動錄製鉤子）、場景的 `Update / Draw`，以及延後指令的套用。它回傳 `RunOutcome`（`Quit` 或 `Restart`），讓 `main.cpp` 可區分兩種結束語意：`Restart` 意味重建整個場景堆疊、回到標題；`Quit` 意味最終拆除程序。

自動錄製鉤子的位置分工：`MaybeAttach` 留在 `main.cpp`、`WireEvents` 移入 `GameplayScene::Enter()`、而 `BeginFrame / EndFrame` 封裝在此處的 `Run` 迴圈中——這樣可確保每幀的錄製接縫不分場景切換與否都維持逐幀完整。

`SceneManager` 不可複製（顯式 `= delete`），強調每個程序只有一個主迴圈。

## 關鍵內容（類別 / 函式 / 資料）

- **`SceneManager`**：場景堆疊管理器。
  - `Push(std::unique_ptr<IScene> scene) → void`：在 `Run` 之前推入首個場景（由 composition root 呼叫）。
  - `Run(Window&, IRenderer&, Harness&) → RunOutcome`：全程式唯一主迴圈，整合 DrawScope / BeginFrame / EndFrame / Update / Draw / ApplyCommand。
  - `Empty() const noexcept → bool`：查詢堆疊是否為空。
  - `RunOutcome`（enum）：`Quit`（最終結束）/ `Restart`（重建場景堆疊）。
  - `ApplyCommand(SceneCommand) → StepResult`（私有）：在 Draw / EndFrame 後套用延後指令；`StepResult` 為 `Continue / Quit / Restart`。
- **`stack_`**（`std::vector<std::unique_ptr<IScene>>`）：場景堆疊，只有 `back()` 為現役。

## 相依與在架構中的位置

- **#include（往外）**：`include/app/IScene.h`（場景介面 + `SceneCommand`）；另前向宣告 `IRenderer`、`Window`、`Harness`，不拉進具體實作。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`、`src/app/SceneManager.cpp`（實作）、`src/app/main.cpp`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：app 層的「主迴圈驅動者」；在 MVC 架構之外，是 Model + View + Controller 三件組的容器框架。`Run` 的每幀迭代就是整個系統推進的心跳。

## OO 概念與設計重點

`SceneManager` 是一個有限狀態機（FSM）：場景堆疊的 Push / Replace / Pop / Restart / Quit 就是狀態轉換，`Run` 迴圈是驅動器。它體現了 [State 模式](../concepts/pat-state.md) 的堆疊變體（每個 `IScene` 是一個狀態，`SceneManager` 是 context）。

延後指令套用（`ApplyCommand` 在 Draw 之後才執行）是 [Command 模式](../concepts/pat-command.md) 的應用：`SceneCommand` 是帶有工廠 thunk 的指令物件，讓指令的發出（Update 回傳）與執行（ApplyCommand）解耦，避免幀中途的狀態不一致。

`RunOutcome` + `main.cpp` 重建場景堆疊的設計，使「重新開始」觸發整個 World / View / GameController 範圍的 RAII 重建（[RAII](../concepts/oo-raii.md)），避免了跨局殘留的狀態污染。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/SceneManager.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/SceneManager.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Command](../concepts/pat-command.md) · [RAII](../concepts/oo-raii.md)
