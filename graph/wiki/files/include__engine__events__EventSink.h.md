---
id: file:include/engine/events/EventSink.h
type: header
path: include/engine/events/EventSink.h
domain: engine
bucket: events
loc: 39
classes: []
sources: ["include/engine/events/EventSink.h"]
---
# `EventSink.h`

> **一句定位**：實體層發布事件的可替換接縫（seam），讓道具 / NPC / Vendor 透過 `nccu::events::Sink()` 發布事件而非直接點名 `EventBus::Instance()`，使測試可將接收端切換為區域匯流排以求隔離。

## 職責

`EventSink.h` 定義了兩個函式：`SetSink(EventBus*)` 和 `Sink()`，構成「實體層發布事件的程式接縫」。

問題背景：實體層（傘家族 / 消耗品 / NPC / 拾取物 / Vendor / BuildingTracker）需要發布遊戲事件，但若直接呼叫 `EventBus::Instance().Publish(...)`，任何改用區域 `EventBus` 的測試都要透過同一個全域 Singleton，難以隔離。若改為把 `EventBus&` 參數串通每個 `Interact / Consume / OnPickup`，則會污染 `IInteractable` 等角色介面簽章與所有測試夾具。

解法：讓實體改透過 `nccu::events::Sink().Publish(Event{...})` 發布。`main.cpp / GameController` 在啟動時呼叫 `SetSink(&bus)` 一次，使 `Sink()` 指向正確的匯流排。測試可呼叫 `SetSink(&localBus)` 將接收端切換為區域匯流排，實現隔離；測試結束後呼叫 `SetSink(nullptr)` 退回 `EventBus::Instance()`，維持既有行為不變。

當接縫保持 `null`（預設），`Sink()` 退回 `EventBus::Instance()`——所有既有呼叫點行為完全不變，直到明確改向為止。這與 `Input::SetSource` / `Time::SetFixedStep` 的測試接縫設計思路完全對應。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::events::SetSink(EventBus* bus) noexcept → void`**：設定當前事件接收端；傳 `nullptr` 退回 `EventBus::Instance()`。
- **`nccu::events::Sink() noexcept → EventBus&`**：取得當前事件接收端（已設定者優先，否則 `EventBus::Instance()`）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/events/EventBus.h`（`EventBus` 型別定義）。
- **被誰使用（往內）**：幾乎所有實體層 `.cpp`（傘家族 / 消耗品 / NPC / Vendor / BuildingTracker 等）及 `src/app/main.cpp`（在啟動時 `SetSink`）、`src/app/scenes/GameplayScene.cpp`（場景啟動時 `SetSink`）；`src/engine/events/EventSink.cpp`（實作）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/events 層；不直接參與管線，是「游離在 Singleton 之外的可替換接縫」，讓 engine 層與 game 層的事件發布可測試。

## OO 概念與設計重點

`EventSink` 是一個「測試 seam」（接縫）設計——與 `Input::SetSource` 和 `Time::SetFixedStep` 同形。它讓 [Singleton](../concepts/pat-singleton.md)（`EventBus`）不致完全阻礙單元測試：測試可用 `SetSink` 暫時切換接收端，觀察與驗證實體的事件發布行為，而無需依賴全域狀態。

與直接注入 `EventBus&` 參數相比，此接縫設計避免了簽章污染（角色介面 / 建構子不需要額外參數），代價是一個程序級的可變靜態指標——這是 Singleton 系統中常見的「最小侵入性測試手法」。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/events/EventSink.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventSink.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [Singleton](../concepts/pat-singleton.md)
