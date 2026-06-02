---
id: file:include/engine/platform/Harness.h
type: header
path: include/engine/platform/Harness.h
domain: engine
bucket: platform
loc: 97
classes: [Harness]
sources: ["include/engine/platform/Harness.h"]
---
# `Harness.h`

> **一句定位**：自動遊玩／觀測載具的主控制代碼，以腳本化輸入和固定時間步長無頭驅動遊戲，逐幀截圖與輸出狀態 JSONL，是整個 [arch-harness](../concepts/arch-harness.md) 可決定性重播框架的入口。

## 職責

`Harness` 是腳本化執行的「感知＋操作」接縫。它透過 `MaybeAttach()` 自由函式（讀取 `UMBRELLA_SCRIPT` 環境變數）來決定是否啟用：預設關閉，正常遊玩逐位元不變；啟用時安裝 `ScriptInput`（透過 `Input::SetSource`）與固定時間步長（透過 `Time::SetFixedStep`）。

主要功能（均在 `HarnessState` pimpl 中實作）：
- **`BeginFrame()`**：推進 `ScriptInput` 的傳統時間軸 (`Advance()`) 並解析當前計畫步驟 (`ResolvePlan()`)，讓本幀的輸入在 `GameController` 讀取前就就緒。
- **`EndFrame(const World& world)`**：擷取世界快照供下幀計畫解析；若配置了 `UMBRELLA_SHOTS`，每 `UMBRELLA_SHOT_EVERY` 幀呼叫 raylib `TakeScreenshot`；若配置了 `UMBRELLA_STATE`，把玩家位置 / 旗標 / 事件序列化為一行 JSON 附加到狀態檔。
- **`WireEvents()`**：向 `EventBus` 訂閱以蒐集事件；須在 `GameController` 建構後呼叫，維持解構順序安全（控制器解構時清除匯流排）。
- **`ShouldQuit()`**：腳本下達 quit 或 `UMBRELLA_MAXFRAMES` 看門狗觸發後回傳 true，讓 `SceneManager::Run` 或 `main.cpp` 的退出條件能無頭終止執行。
- **`SpritePath()`**：啟用時回傳 `UMBRELLA_SPRITE` 指定的角色 sprite 路徑，使選角步驟完全略過（可決定性選角）。

pimpl 設計（`unique_ptr<HarnessState> s_`）：`HarnessState` 定義在 `.cpp` 中，`Harness` 可移動但不可複製；此設計確保 `EventBus` 捕獲的是指向 `HarnessState` 的穩定指標，跨移動不失效。

腳本語法（兩種形式可交錯，詳見 `ScriptInput.h`）：傳統計時（行首為幀號）和高階計畫動詞（`goto / interact / choose / advance / wait / quit`）。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::Harness`**：自動遊玩載具。
  - `Active() const noexcept → bool`：載具是否啟用（有腳本）。
  - `ShouldQuit() const noexcept → bool`：是否應結束本局（腳本 quit 或看門狗）。
  - `SpritePath() const → string`：固定角色 sprite 路徑（啟用時）。
  - `WireEvents()`：向 EventBus 訂閱事件蒐集。
  - `BeginFrame()`：每幀在 Update 前：推進時間軸 + 解析計畫。
  - `EndFrame(const World& world)`：每幀在 EndDrawing 後：快照 + 截圖 + 狀態輸出。
  - `s_`（`unique_ptr<HarnessState>`）：pimpl；隱藏所有狀態，保持標頭的可編譯性（`HarnessState` 引入 raylib 等只能在 `.cpp` 中的依賴）。
- **`nccu::MaybeAttach() → Harness`**：自由函式，讀環境變數並回傳已啟用或未啟用的 `Harness`；`main.cpp` 呼叫一次。

## 相依與在架構中的位置

- **#include（往外）**：`<memory>`, `<string>`（標準庫）；不包含任何 raylib / engine / game 標頭（pimpl 使細節留在 `.cpp`）——這正是 pimpl 的優點。
- **被誰使用（往內）**：`src/app/SceneBootstrap.cpp`、`src/app/SceneManager.cpp`、`src/app/main.cpp`（擁有 Harness 實例）、`src/app/scenes/GameplayScene.cpp`（借用參考，`WireEvents`）、`src/engine/platform/Harness.cpp`（實作）。
- **繼承 / 實作 / 體現**：realizes [決定性 autoplay（Harness）](../concepts/arch-harness.md)。
- **每幀管線 / MVC 角色**：engine/platform 層；`BeginFrame` 在每幀管線的「輸入讀取前」執行，`EndFrame` 在「EndDrawing 後」執行，包覆整個幀但不干擾 Model / View / Controller 內部的邏輯。

## OO 概念與設計重點

pimpl idiom（`unique_ptr<HarnessState>`）隱藏了 `HarnessState` 對 raylib / `ScriptInput` / `EventBus::Subscription` 等的依賴，使 `Harness.h` 本身保持輕量可包含。pimpl 同時讓 `EventBus` handler 捕獲的 `HarnessState*` 在 `Harness` 被移動時仍然有效（指標不變）。

[arch-harness](../concepts/arch-harness.md) 三件套（`Input::SetSource / Time::SetFixedStep / ScriptInput`）的統一入口：`Harness` 在 `MaybeAttach` 時一起安裝三件套，在解構時還原，確保正常遊玩路徑零副作用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/Harness.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Harness.h) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [RAII](../concepts/oo-raii.md)
