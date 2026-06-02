---
id: "file:src/engine/audio/AudioDevice.cpp"
type: source
path: src/engine/audio/AudioDevice.cpp
domain: engine
bucket: audio
loc: 13
classes: []
sources: ["src/engine/audio/AudioDevice.cpp"]
---
# `AudioDevice.cpp`

> **一句定位**：`AudioDevice` 的 RAII 骨架實作：目前以 `= default` 佔位，預留未來啟用 `::InitAudioDevice()` / `::CloseAudioDevice()` 時無須更動呼叫端的介面。

## 職責

此檔案提供 `AudioDevice` 建構子與解構子的 `= default` 定義。目前為空殼——`ready_` 永遠為 false，不呼叫 raylib 的 `::InitAudioDevice()`，也無任何音效資產。

設計意圖：當第一個音訊素材加入時，建構子填入 `::InitAudioDevice()` 並設 `ready_ = true`，解構子在 `Ready()` 為真時對稱地呼叫 `::CloseAudioDevice()`。標頭已宣告「不可複製、不可移動、RAII、`Ready()` 存取器」——函式體補上時不需更動 `main()` 或任何呼叫點，符合 Open-Closed 設計。

程式層級的唯一 RAII handle：`main.cpp` 以值宣告 `nccu::audio::AudioDevice audioDevice`，在 composition root 擁有其完整生命期，解構與 Window / Font 排在同一層，保證有序拆除。

## 關鍵內容（類別 / 函式 / 資料）

- `AudioDevice::AudioDevice() noexcept = default` — 空殼建構子；未來填入 `::InitAudioDevice()` + `ready_ = true`。
- `AudioDevice::~AudioDevice() noexcept = default` — 空殼解構子；未來填入 `if (Ready()) ::CloseAudioDevice()`。

## 相依與在架構中的位置
- **#include（往外）**：`AudioDevice.h`（唯一依賴）
- **被誰使用（往內）**：—（葉節點；由 `main.cpp` 與 `AudioManager.cpp` 直接持有/引用）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / audio 層；程式級資源，不在每幀模擬管線中執行

## OO 概念與設計重點

[RAII](../concepts/oo-raii.md) 設計：音訊裝置的初始化與釋放封裝在建構/解構子配對，使呼叫端（`main.cpp`）不需手動呼叫 init/close。「先聲明介面、後填函式體」的骨架方法讓 OCP（Open-Closed Principle）落實在 C++ 檔案層次——介面穩定、實作演進。`noexcept` 標記確保解構子不拋例外，符合 C++ 核心準則。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/audio/AudioDevice.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/audio/AudioDevice.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
