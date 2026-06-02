---
id: "file:src/engine/audio/AudioManager.cpp"
type: source
path: src/engine/audio/AudioManager.cpp
domain: engine
bucket: audio
loc: 22
classes: []
sources: ["src/engine/audio/AudioManager.cpp"]
---
# `AudioManager.cpp`

> **一句定位**：`AudioManager` 的骨架實作：保留 `(EventBus&, AudioDevice&)` 參考以備日後安裝音效訂閱者，目前尚無任何音效資產，不安裝訂閱。

## 職責

`AudioManager` 是未來「玩法事件 → 音效播放」的橋接點。建構子接收 `EventBus& bus` 與 `AudioDevice& device` 兩個參考，以成員保存（`bus_`、`device_`），但目前以 `(void)bus_; (void)device_` 抑制未使用警告——無任何 `Subscribe` 呼叫。

設計意圖：日後的音效訂閱者可直接在建構子安裝，解構子能在同一個 RAII 保證下取消訂閱，與 `GameController` 一致（重新開始時先清空再重建，確保 EventBus handler 不變成懸空）。

在 `GameplayScene` 中排在成員列表最後，使其解構時最先拆除音訊——保證對應的 `AudioDevice` 仍有效（AudioDevice 由 `main.cpp` 擁有，生命期更長）。

## 關鍵內容（類別 / 函式 / 資料）

- `AudioManager::AudioManager(EventBus& bus, AudioDevice& device) noexcept` — 保存參考；`(void)` 抑制警告；目前不安裝訂閱。
- `AudioManager::~AudioManager() noexcept = default` — 空殼解構子；日後在此取消訂閱。

## 相依與在架構中的位置
- **#include（往外）**：`AudioManager.h`、`AudioDevice.h`（持有參考）
- **被誰使用（往內）**：—（由 `GameplayScene` 持有為成員）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / audio 層；事件訂閱骨架，不在每幀模擬管線中執行

## OO 概念與設計重點

此檔案是 [Observer](../concepts/pat-observer.md) 模式的預留接縫：`EventBus` 的 `Subscribe` / `ScopedSubscribe` 管道已就位，一旦有音效資產即可以零呼叫端變更的方式接入。[RAII](../concepts/oo-raii.md) 的解構子預留點確保訂閱者的生命期綁定於 `GameplayScene` 的單次遊玩範圍，重啟時乾淨拆除。`noexcept` 建構子與解構子符合 C++ 核心準則，也讓 `GameplayScene` 的初始化列更安全。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/audio/AudioManager.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/audio/AudioManager.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
