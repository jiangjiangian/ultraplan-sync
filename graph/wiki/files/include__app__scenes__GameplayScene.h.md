---
id: file:include/app/scenes/GameplayScene.h
type: header
path: include/app/scenes/GameplayScene.h
domain: app
bucket: scenes
loc: 79
classes: [GameplayScene]
sources: ["include/app/scenes/GameplayScene.h"]
---
# `GameplayScene.h`

> **一句定位**：單局遊玩的 app 層場景，封裝 World / View / GameController / AudioManager 四件組的所有權與每幀推進，並暴露 `WorldForHarnessOrNull()` 給自動錄製機制。

## 職責

`GameplayScene final : public IScene` 是遊戲核心的「組裝容器」，其最重要的職責是持有並正確排序四個成員的生命週期：`World`（Model）、`View`（View）、`GameController`（Controller）、`AudioManager`（音訊調度器）。成員宣告順序具語意——C++ 逆序解構保證：AudioManager 最先拆除（維持 EventBus 訂閱者不懸空）、再是 GameController（`EventBus::Clear`）、最後才是被訂閱者捕獲的 `World`。

`Harness&` 與 `AudioDevice&` 以參考方式借用——兩者的生命週期橫跨整個程式，由 `main.cpp` 擁有，因此場景切換時不會被誤解構。

`Enter()` 是場景接線時機，負責呼叫 `harness.WireEvents()`（需在 `GameController` 建構後才安全呼叫，以維持解構順序安全）和訂閱必要的事件。`Update` 從 `World::PendingAppAction()` 判斷是否需要 Restart / Quit：`Restart` 回傳 `SceneCommand{Restart}`，`main.cpp` 據此重建整個場景堆疊（觸發 RAII 全面重建）；`Quit` 回傳 `SceneCommand{Quit}`，觸發程序結束。若 `RestartFactory` 為空（自動錄製路徑），Restart 被導向 Quit，對應「腳本化執行永不重啟」的合約。

`WorldForHarnessOrNull()` 覆寫以回傳 `&world_`，讓 `SceneManager::Run` 在每幀 `EndFrame(world)` 時能傳入有效的 `World` 快照。

## 關鍵內容（類別 / 函式 / 資料）

- **`GameplayScene`**（`final : IScene`）：遊玩場景。
  - `RestartFactory`（型別別名）：`std::function<std::unique_ptr<IScene>()>`，重啟時重建場景鏈；空表示「無重啟路徑」。
  - `GameplayScene(CharacterSelectResult, AudioDevice&, Harness&, int, int, RestartFactory)`：建構並初始化四件組。
  - `Enter() override`：呼叫 `harness.WireEvents()` 並訂閱事件。
  - `Update(float dt) → SceneCommand override`：推進 `GameController`，根據 `World::PendingAppAction()` 回傳切換指令。
  - `Draw(IRenderer&) override`：調用 `View` 繪製完整畫面。
  - `Exit() override`：取消訂閱 / 清理。
  - `WorldForHarnessOrNull() const noexcept → const World* override`：回傳 `&world_`，供錄製機制使用。
  - `world_`（`nccu::World`）：Model，擁有所有遊戲狀態。
  - `view_`（`nccu::View`）：View，渲染世界。
  - `controller_`（`nccu::GameController`）：Controller，收輸入、跑系統管線。
  - `audioManager_`（`nccu::audio::AudioManager`）：音訊事件調度器。
  - `harness_`（`nccu::Harness&`）：借用；自動錄製載具。
  - `restartFactory_`（`RestartFactory`）：重啟工廠 closure。

## 相依與在架構中的位置

- **#include（往外）**：`include/app/IScene.h`、`include/game/controller/GameController.h`、`include/engine/audio/AudioDevice.h`、`include/engine/audio/AudioManager.h`、`include/ui/CharacterSelect.h`（角色選取結果型別）、`include/ui/View.h`、`include/game/world/World.h`。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`（由工廠鏈建構）、`src/app/scenes/GameplayScene.cpp`（實作）。
- **繼承 / 實作 / 體現**：繼承並實作 `IScene`。
- **每幀管線 / MVC 角色**：app 層的「遊玩場景」，組裝並驅動 MVC 三件組；`Update` 觸發 GameController 的每幀管線（Survival → Movement → Collision → Spawn → Interact → Sweep）；`Draw` 觸發 View 的繪製。

## OO 概念與設計重點

成員宣告順序（`world_ → view_ → controller_ → audioManager_`）是 C++ RAII 解構順序保證的核心設計，讓四件組的拆除順序完全正確，無需手動撰寫 destructor——這是 [RAII](../concepts/oo-raii.md) 的精髓應用。

`RestartFactory` closure 使 `GameplayScene` 不知道「重啟後建什麼場景」，符合依賴倒置。`WorldForHarnessOrNull()` 利用 `IScene` 的虛擬函式接縫（非 `dynamic_cast`）暴露 `World*`，維持 [arch-harness](../concepts/arch-harness.md) 的無侵入設計。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/GameplayScene.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/GameplayScene.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md) · [MVC](../concepts/arch-mvc.md) · [Harness](../concepts/arch-harness.md)
