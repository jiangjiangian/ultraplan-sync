---
id: file:include/engine/input/Input.h
type: header
path: include/engine/input/Input.h
domain: engine
bucket: input
loc: 70
classes: [InputSource, LiveInput, Input]
sources: ["include/engine/input/Input.h"]
---
# `Input.h`

> **一句定位**：多型輸入來源抽象層，以靜態門面 `Input` 集中全程序按鍵查詢，透過 `SetSource` 支援腳本化無頭重播，是 [Harness](../concepts/arch-harness.md) 可決定性的核心接縫。

## 職責

此標頭定義三個型別，共同構成「多型輸入來源」系統：

**`InputSource`**（抽象介面）：三個純虛擬函式 `IsDown / IsPressed / IsReleased`，為所有按鍵查詢的多型合約。

**`LiveInput`**（具體類別，`final`）：預設的真實輸入實作，直接呼叫 raylib 的 `IsKeyDown / IsKeyPressed / IsKeyReleased`（透過 `ToRaylibKey` 轉換 `Key` 列舉值）。

**`Input`**（靜態門面）：全程序的唯一按鍵查詢入口。
- `SetSource(InputSource*)` 安裝可替換來源（`nullptr` 退回 `LiveInput`）。
- `Source()` 回傳當前來源（若 `current_` 為 null 則回傳 function-local static `LiveInput`）。
- `IsDown / IsPressed / IsReleased` 三個靜態函式轉發至 `Source()`。

此設計讓 `Player / GameController / CharacterSelectScene / TitleScene` 等所有按鍵讀取都收斂至 `Input` 這一個點，替換來源（如安裝 `ScriptInput`）不需改任何呼叫點。自動遊玩 `Harness` 在局開始時呼叫 `Input::SetSource(&scriptInput)`，局結束時傳 `nullptr` 還原；正常遊玩逐位元不變。

`current_`（`inline static InputSource*`）是實現 seam 的機制：C++17 inline static 在 header 即可定義，不需對應的 `.cpp`。

## 關鍵內容（類別 / 函式 / 資料）

- **`InputSource`**（抽象）：`IsDown / IsPressed / IsReleased(Key) const noexcept = 0`。
- **`LiveInput`**（`final : InputSource`）：三個方法呼叫 `::IsKeyDown / ::IsKeyPressed / ::IsKeyReleased(ToRaylibKey(k))`。
- **`Input`**（靜態門面）：
  - `SetSource(InputSource*) noexcept`：安裝可替換來源（測試 seam）。
  - `Source() noexcept → InputSource*`：取得當前來源（null 退回 LiveInput static）。
  - `IsDown / IsPressed / IsReleased(Key) noexcept → bool`：靜態轉發函式。
  - `current_`（`inline static InputSource*`，初值 `nullptr`）：當前來源指標；`Input` 不擁有它。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`LiveInput` 呼叫 raylib 按鍵查詢）、`include/engine/input/Key.h`（型別安全按鍵列舉）。
- **被誰使用（往內）**：`include/engine/platform/ScriptInput.h`（繼承 `InputSource`）、`include/game/controller/InputHandler.h`；眾多場景、控制器、實體及測試 `.cpp`。
- **繼承 / 實作 / 體現**：`LiveInput` 實作 `InputSource`；`ScriptInput` 同樣繼承 `InputSource`；多個測試繼承 `InputSource` 做 stub（`test_i35_interact_vendor` 等）。
- **每幀管線 / MVC 角色**：engine/input 層；在 Controller（`GameController`）的每幀 Update 開始時查詢輸入，是 [arch-harness](../concepts/arch-harness.md) 可決定性的輸入接縫。

## OO 概念與設計重點

`Input` + `InputSource` 是「策略模式的靜態門面化」——[Strategy 模式](../concepts/pat-strategy.md) 的靜態變體：`InputSource` 是策略介面，`LiveInput` / `ScriptInput` 是具體策略，`Input::SetSource` 是策略切換點，而靜態門面讓呼叫端不需持有策略物件的參考。

與 `EventSink::SetSink` / `Time::SetFixedStep` 相同形式的測試接縫（[arch-harness](../concepts/arch-harness.md)），構成整個 Harness 可決定性重播的三件套基礎設施。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/input/Input.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/input/Input.h) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [Harness](../concepts/arch-harness.md)
