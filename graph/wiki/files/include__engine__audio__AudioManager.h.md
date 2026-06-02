---
id: file:include/engine/audio/AudioManager.h
type: header
path: include/engine/audio/AudioManager.h
domain: engine
bucket: audio
loc: 48
classes: [AudioManager]
sources: ["include/engine/audio/AudioManager.h"]
---
# `AudioManager.h`

> **一句定位**：單局音訊調度器，透過 EventBus 訂閱將遊戲事件轉成音訊播放，目前為空殼預留框架，生命週期與 GameplayScene 的單局範圍嚴格綁定。

## 職責

`AudioManager` 是遊戲音訊的事件驅動調度器。它持有 `EventBus&` 和 `AudioDevice&` 的參考，在建構時安裝事件訂閱，在解構時取消訂閱，避免懸空 handler 或重開時的旗標污染。

目前所有函式體均為空殼——專案尚未含音訊素材，故不訂閱任何事件，也不播放任何音效；但類別形狀已完整，未來只需在建構子內訂閱 `EventType::UmbrellaClaimed / KarmaChanged / EnteredBuilding / PickupAcquired / ShowMessage` 等事件，並在對應 handler 中呼叫 raylib 音訊 API 即可，無須重組 `GameplayScene`。

生命週期模型比照 `GameController`：與 `World / View / Controller` 三件組一起在 `GameplayScene` 中建構，逆序解構時最先被拆除（`AudioManager` 宣告於 `controller_` 之後，逆序解構使它比 `controller_` 更早析構），保持 EventBus 訂閱者在 `World` 死亡前先行清除的紀律。

不可複製、不可移動：每局唯一，錨定於擁有它的場景範圍。

## 關鍵內容（類別 / 函式 / 資料）

- **`AudioManager`**：單局音訊調度器。
  - `AudioManager(::EventBus& bus, AudioDevice& device) noexcept`：以事件匯流排與音訊裝置建構（目前空殼，未安裝訂閱）。
  - `~AudioManager() noexcept`：解構；將在此取消所有事件訂閱。
  - `bus_`（`::EventBus&`）：訂閱事件的來源匯流排。
  - `device_`（`AudioDevice&`）：音訊裝置控制代碼。
  - 複製與移動均顯式 `= delete`。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/events/EventBus.h`（全域 `::EventBus` 單例，用於訂閱遊戲事件）。
- **被誰使用（往內）**：`include/app/scenes/GameplayScene.h`（成員）、`src/engine/audio/AudioManager.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine 層；在 `GameplayScene` 中與 `GameController` 並列，不直接參與每幀模擬管線，而是透過 `EventBus` 被動接收事件。

## OO 概念與設計重點

`AudioManager` 體現了 [Observer 模式](../concepts/pat-observer.md)（訂閱端）：將音訊播放邏輯解耦於遊戲事件的發布者（道具 / 狀態機），使兩者互不知曉。RAII 建構 / 解構管理訂閱生命週期，避免懸空 handler——與 `GameController` 的相同設計紀律一致。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/audio/AudioManager.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/audio/AudioManager.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
