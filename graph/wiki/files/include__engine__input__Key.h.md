---
id: file:include/engine/input/Key.h
type: header
path: include/engine/input/Key.h
domain: engine
bucket: input
loc: 42
classes: []
sources: ["include/engine/input/Key.h"]
---
# `Key.h`

> **一句定位**：型別安全的按鍵列舉，將引擎使用的所有按鍵對映到 raylib KEY_* 整數值，避免裸整數鍵碼散落各處。

## 職責

`Key.h` 定義 `nccu::engine::input::Key`——一個底層型別為 `int` 的 `enum class`，列舉值直接等於 raylib 的 `KEY_*` 常數（例如 `Key::Enter = KEY_ENTER`）。搭配 `constexpr` 函式 `ToRaylibKey(Key k) noexcept → int`，讓上層以具名常數操作按鍵，`LiveInput` 在呼叫 raylib 時再用 `ToRaylibKey` 還原為整數。

此設計讓引擎內部（GameController / Player / DialogScreen / InputHandler 等）完全以 `Key::` 具名常數操作按鍵，raylib 的 `KEY_*` 整數僅在 `LiveInput` 的邊界才出現，符合「raylib 限制在引擎層邊界」的架構規則。

涵蓋的鍵：全部 26 個字母鍵（A..Z）、Space、Tab、Enter、Escape、Backspace、四個方向鍵（Up / Down / Left / Right）。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::input::Key`**（`enum class : int`）：具名按鍵列舉，值等於 raylib `KEY_*` 整數（A=65, Enter=257, Up=265 等）。包含 A..Z 26 鍵 + Space / Tab / Enter / Escape / Backspace + 四個方向鍵。
- **`ToRaylibKey(Key k) noexcept → int`**（`constexpr`）：將 `Key` enum 轉回 raylib 整數鍵碼，供 `LiveInput` 使用。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`KEY_*` 整數常數，enum class 值即等於 raylib 鍵碼）——raylib 的滲入限制在此一行。
- **被誰使用（往內）**：`include/engine/input/Input.h`（`InputSource::IsDown` 參數型別）、`include/game/controller/InputHandler.h`；大量場景 / 控制器 / 實體 / 測試 `.cpp`（共 20+ 個使用點）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/input 層的純資料型別；不參與管線，僅作為按鍵識別符。

## OO 概念與設計重點

`enum class : int` + `constexpr ToRaylibKey` 是「型別安全整數別名」的標準 C++ 手法，比 `using Key = int` 更安全（不會與其他整數隱式轉換），比 `enum class` 加複雜轉換更輕量。`constexpr` 保證 `ToRaylibKey` 在 compile time 評估，完全 zero overhead。整個標頭是 header-only，不需 `.cpp`。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/input/Key.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/input/Key.h) · [← 全檔索引](../files-index.md)
