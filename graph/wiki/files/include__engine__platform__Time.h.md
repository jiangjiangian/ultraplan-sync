---
id: file:include/engine/platform/Time.h
type: header
path: include/engine/platform/Time.h
domain: engine
bucket: platform
loc: 36
classes: [Time]
sources: ["include/engine/platform/Time.h"]
---
# `Time.h`

> **一句定位**：幀時間靜態門面，正常遊玩取真實 raylib 幀時間，自動遊玩可透過 `SetFixedStep` 釘定固定步長以求逐幀可重現。

## 職責

`Time` 是一個純靜態結構（無實例化，所有方法均為 `static`），提供全程序統一的幀時間查詢介面。

`DeltaSeconds()` 是核心方法：若 `fixed_` > 0，回傳已釘定的固定步長值；否則呼叫 raylib 的 `::GetFrameTime()` 取得真實幀時間差。正常遊玩 `fixed_` 恆為 0（預設值），行為完全不變。

`SetFixedStep(float s)` 是測試 seam：`Harness` 在啟用時釘定 `1.0f/60.0f`（即 60 fps 的固定步長），讓腳本化執行不受軟體 GL 幀率波動影響，每幀模擬結果完全可重現。`s <= 0` 等同「退回真實幀時間」。

`FpsAvg()` 轉發 `::GetFPS()` 供 HUD 顯示使用，不受 `fixed_` 影響。

`fixed_`（`inline static float`）是 C++17 inline static，header-only 定義，不需對應 `.cpp`。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::platform::Time`**（純靜態結構）：
  - `SetFixedStep(float s) noexcept`：釘定固定步長；`s <= 0` 退回真實幀時間。
  - `DeltaSeconds() noexcept → float`：取得本幀時間差（釘定或真實）。
  - `FpsAvg() noexcept → int`：轉發 `::GetFPS()`。
  - `fixed_`（`inline static float`，初值 0.0f）：釘定值；0 表示用真實幀時間。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`::GetFrameTime()` / `::GetFPS()`）——raylib 使用限制在引擎層。
- **被誰使用（往內）**：`src/app/SceneManager.cpp`（`Run` 迴圈傳 `dt` 給場景）、`src/engine/platform/Harness.cpp`（`SetFixedStep` 啟用固定步長）、`src/game/controller/GameController.cpp`；多個測試（`test_i35_interact_vendor / test_i6_interact_reach / test_dialog_skip / test_rain_survival` 等）直接使用 `Time::SetFixedStep` 在測試中釘定步長。
- **繼承 / 實作 / 體現**：realizes [決定性 autoplay（Harness）](../concepts/arch-harness.md)。
- **每幀管線 / MVC 角色**：engine/platform 層；`DeltaSeconds()` 的結果以 `dt` 參數傳入所有 `IScene::Update / ISystem::Update`，驅動整個每幀模擬管線。

## OO 概念與設計重點

與 `Input::SetSource` 和 `EventSink::SetSink` 同形的「靜態 seam」——[arch-harness](../concepts/arch-harness.md) 三件套之一。靜態門面設計讓呼叫端（`SceneManager / GameController / 各場景`）不需持有 `Time` 物件的參考，直接呼叫靜態函式即可。`inline static` 的 `fixed_` 確保 header-only 且 ODR 安全。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/Time.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Time.h) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
