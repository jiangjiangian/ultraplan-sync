---
id: "file:src/app/SceneManager.cpp"
type: source
path: src/app/SceneManager.cpp
domain: app
bucket: ""
loc: 107
classes: []
sources: ["src/app/SceneManager.cpp"]
---
# `SceneManager.cpp`

> **一句定位**：實作延後套用的場景切換語意與整合 Harness 鉤子的唯一主迴圈（`Run`）。

## 職責

`SceneManager` 管理一個 `std::unique_ptr<IScene>` 的棧（`stack_`）。`Push` 方法在推入新場景時立即呼叫 `Enter()`。`ApplyCommand` 根據 `SceneCommand::Kind` 執行 Push、Replace、Pop、Quit、Restart 五種轉換，並在棧清空時回傳 `StepResult::Quit`，防止程式空轉。

`Run` 是整個遊戲的主迴圈：在每一個迭代中依序執行 `harness.BeginFrame()`（錄製器於此擷取輸入邊緣並解析腳本計畫）、現役場景的 `Update(dt)`、`Draw(renderer)`（包在 `DrawScope` 內）、`harness.EndFrame(world)`（序列化本 tick 可觀察狀態），最後才呼叫 `ApplyCommand` 套用轉換指令。這個先 Draw 再 Apply 的次序確保場景在轉換前完成最後一幀渲染。

Title / Select / Loading 場景回傳 `WorldForHarnessOrNull() == nullptr`，故它們不參與 `EndFrame` 序列化，符合「只有遊玩幀進入 state.jsonl」的設計。`StepResult::Restart` 回傳 `RunOutcome::Restart`，由 `main.cpp` 的外層重建場景鏈。

## 關鍵內容（類別 / 函式 / 資料）

- `Push(unique_ptr<IScene>)` — 推入場景並呼叫 `Enter()`；空指標安全。
- `ApplyCommand(SceneCommand)` — 依 Kind 處理 Push / Replace / Pop / Quit / Restart，棧清空時退化為 Quit。
- `Run(Window& window, IRenderer& renderer, Harness& harness)` — 唯一主迴圈；`BeginFrame → Update → Draw → EndFrame → ApplyCommand`；條件：`!window.ShouldClose() && !harness.ShouldQuit() && !stack_.empty()`。
- `StepResult` enum — `Continue / Quit / Restart`，供 `ApplyCommand` 與 `Run` 之間的內部通訊。
- `RunOutcome` enum — `Quit / Restart`，回傳給 `main.cpp` 決定下一步。

## 相依與在架構中的位置
- **#include（往外）**：`SceneManager.h`、`Harness.h`（BeginFrame/EndFrame/ShouldQuit）、`Time.h`（DeltaSeconds）、`DrawScope.h`（GL Begin/EndDrawing 配對）、`Window.h`（ShouldClose）、`World.h`（WorldForHarnessOrNull 的回傳型別）
- **被誰使用（往內）**：—（葉節點 / 組裝根；由 `main.cpp` 持有並呼叫 `Run`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：app 層；主迴圈外殼，協調 Harness（BeginFrame/EndFrame）與各 IScene 的 Update/Draw，不直接參與 Survival→…→Sweep 的遊戲模擬管線

## OO 概念與設計重點

`SceneManager::Run` 體現 [Harness](../concepts/arch-harness.md) 的框架合約：`BeginFrame` / `EndFrame` 必須在相同的場景 tick 配對內執行，確保輸入邊緣與序列化狀態的逐位元一致性。`DrawScope` 採用 RAII（[RAII](../concepts/oo-raii.md)）管理 raylib 的 `Begin/EndDrawing` 配對，使任何提前返回路徑（Replace 後棧為空）都不會留下不配對的 GL 呼叫。延後套用（先完成 Draw 再 ApplyCommand）是一個精心設計的邊界條件，防止場景在它自身的最後一幀還沒畫完就被銷毀。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/SceneManager.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/SceneManager.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [RAII](../concepts/oo-raii.md) · [MVC](../concepts/arch-mvc.md)
