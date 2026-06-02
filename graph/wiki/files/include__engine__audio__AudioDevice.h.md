---
id: file:include/engine/audio/AudioDevice.h
type: header
path: include/engine/audio/AudioDevice.h
domain: engine
bucket: audio
loc: 49
classes: [AudioDevice]
sources: ["include/engine/audio/AudioDevice.h"]
---
# `AudioDevice.h`

> **一句定位**：引擎音訊裝置的 RAII 控制代碼，目前為空殼預留位，確保音訊系統的生命週期框架在資源加入前就已就位。

## 職責

`AudioDevice` 是一個不可複製、不可移動的 RAII 型別，代表音訊裝置的唯一控制代碼。目前建構子與解構子均為空殼（不呼叫 raylib 的 `InitAudioDevice` / `CloseAudioDevice`），因為專案尚未包含任何音訊素材。

保留此類別形狀的理由有三：第一，`main()` 已在正確的位置（在 `Window` 建構後、在 GL 相關資源之前）預留音訊裝置的建構與解構槽位；第二，當第一個音訊素材加入時，只需讓建構子呼叫 `InitAudioDevice()`、解構子呼叫 `CloseAudioDevice()`，無須重組 `main()` 的初始化順序；第三，空殼建構子沒有任何 PRNG / 計時副作用，使自動錄製的對照輸出維持逐位元一致（可決定性保證）。

`Ready()` 回傳 `bool`，目前恆為 `false`（因 `ready_` 永遠不被設成 `true`）；當裝置真正啟用後翻轉為 `true`，呼叫點無需修改。

## 關鍵內容（類別 / 函式 / 資料）

- **`AudioDevice`**：音訊裝置 RAII 控制代碼。
  - `AudioDevice() noexcept`：建構音訊裝置（目前空殼，未呼叫 `InitAudioDevice`）。
  - `~AudioDevice() noexcept`：解構（裝置啟用後於此呼叫 `CloseAudioDevice`）。
  - `Ready() const noexcept → bool`：裝置是否已啟用；目前恆回傳 `false`。
  - `ready_`（`bool`，初值 `false`）：裝置啟用旗標。
  - 複製與移動均顯式 `= delete`（每程序唯一）。

## 相依與在架構中的位置

- **#include（往外）**：無（不依賴任何其他標頭）——刻意隔離，使 engine/audio 層不滲入 app 或 game 層的標頭樹。
- **被誰使用（往內）**：`include/app/scenes/GameplayScene.h`（借用參考）、`src/app/SceneBootstrap.cpp`、`src/app/main.cpp`（擁有實例）、`src/engine/audio/AudioDevice.cpp`（實作）、`src/engine/audio/AudioManager.cpp`（注入依賴）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine 層；不參與每幀管線，僅在程式啟動 / 結束時建構 / 解構一次。

## OO 概念與設計重點

典型的 [RAII](../concepts/oo-raii.md)「資源代理」模式：即使目前函式體為空，物件形狀已確保「建構即取得裝置、解構即釋放裝置」的契約，日後只需填入實作而無需改動任何使用端。不可複製 + 不可移動確保每程序唯一的語意。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/audio/AudioDevice.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/audio/AudioDevice.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
