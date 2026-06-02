---
id: "file:tests/controller/test_input_handler.cpp"
type: test
path: tests/controller/test_input_handler.cpp
domain: tests
bucket: controller
loc: 203
classes: [StubInput, ScopedInputSource]
sources: ["tests/controller/test_input_handler.cpp"]
---
# `test_input_handler.cpp`

> **一句定位**：以 `StubInput` 注入確定性按鍵狀態，驗證 `InputHandler` 的 E 鍵 edge 觸發、長按計時門檻（300 ms）、冷卻節奏（4 格）、計時重置，以及 edge 與 auto 在同一格不重複觸發的守門邏輯。

## 職責

本檔為 `InputHandler`（從 `GameController` 抽出的 E 鍵邏輯）提供完整的隔離單元測試。兩個輔助型別建立測試 fixture：

**`StubInput`**：以三個 `uint32_t` 位元掩碼（`down_`、`pressed_`、`released_`）模擬 raylib 的按鍵邊緣語意。`Tap` 設置 `down + pressed`；`NextFrame` 清除 `pressed/released`，保留 `down`（按住跨格）；`Release` 清除 `down` 並設 `released`。`Bit(Key)` 以 `raw & 31` 打包，對單鍵測試無碰撞。

**`ScopedInputSource`**：RAII 包裝，ctor 呼叫 `Input::SetSource`，dtor 還原為 null（LiveInput），確保每個 case 隔離。

六個 `TEST_CASE` 覆蓋：
1. **單格 tap 觸發一次**：`TickDialogAdvance(kFrameDt)` 回傳 true，`HoldMs > 0`，`Cooldown == 0`。
2. **長按未滿 300 ms 不自動推進**：16 格（≈266 ms）按住後 `HoldMs < kHoldAdvanceMs`，每格皆回傳 false。
3. **長按超過 300 ms 自動推進**：40 格（≈640 ms）後 `autoCount >= 3`，確認 `kAutoCooldownFrames == 4`。
4. **放開重置計時**：按住後放開，`HoldMs == 0`；再次 tap 觸發全新 edge，不累積先前按住時間。
5. **`ResetDialogAdvance` 清除計時與冷卻**：模擬對話關閉後 Controller 呼叫 Reset 的路徑；`HoldMs == 0`、`Cooldown == 0`。
6. **edge 優先於 auto 不重複觸發**：預先累積長按計時，再強制一次 edge-press，`TickDialogAdvance` 回傳 true 但冷卻戳記不被重設，確認單次按下絕不把對話推進兩行。

## 關鍵內容（類別 / 函式 / 資料）

- `class StubInput : InputSource`：`IsDown/IsPressed/IsReleased` 的位元掩碼實作；`Tap/Hold/Release/NextFrame`。
- `class ScopedInputSource`：RAII 全域 InputSource 切換守衛。
- `constexpr float kFrameDt = 1.0f / 60.0f`：固定格步進。
- `nccu::InputHandler`（被測型別）：`TickDialogAdvance(dt)` → `bool`；`HoldMs()`、`Cooldown()`、`ResetDialogAdvance()`。
- `InputHandler::kHoldAdvanceMs`（300 ms 門檻）、`InputHandler::kAutoCooldownFrames`（4 格冷卻）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/controller/InputHandler.h`（被測主體）、`include/engine/input/Input.h`、`include/engine/input/Key.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`StubInput` 繼承 `InputSource`（`include/engine/input/Input.h`）
- **每幀管線 / MVC 角色**：對 `GameController` Update 前段的輸入處理層進行隔離測試

## OO 概念與設計重點

`StubInput` 體現 [Strategy](../concepts/pat-strategy.md) 模式：`InputHandler` 透過 `Input::SetSource` 注入，完全不知道底層是真實 raylib 還是測試 stub。`ScopedInputSource` 體現 [RAII](../concepts/oo-raii.md)，確保全域狀態（`Input::SetSource`）在每個 case 後必然還原，不論 case 是否拋出例外。六個 case 以固定時鐘（`kFrameDt`）確保數值確定性，不依賴牆鐘。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_input_handler.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_input_handler.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [RAII](../concepts/oo-raii.md)
