---
id: "file:src/engine/events/EventBus.cpp"
type: source
path: src/engine/events/EventBus.cpp
domain: engine
bucket: events
loc: 86
classes: []
sources: ["src/engine/events/EventBus.cpp"]
---
# `EventBus.cpp`

> **一句定位**：`EventBus` Singleton + Observer 的完整實作：執行緒安全的 Subscribe / ScopedSubscribe / Publish / Unsubscribe / Clear，以及 RAII `Subscription` 的移動語意與 Reset。

## 職責

`EventBus` 是全域事件匯流排（Singleton），以靜態局部變數（`static EventBus instance`）保證唯一實例與執行緒安全初始化。

`Subscribe` 以 `unique_lock` 加入 `Slot{nextId_, handler}` 至 `handlers_[type]`；`ScopedSubscribe` 相同但回傳 `Subscription` RAII 物件，解構時自動 `Unsubscribe(id)`。

`Publish` 採「快照後派送」策略：先以 `shared_lock` 複製該 EventType 的 handler 清單（確保快照穩定），釋放鎖後再逐一呼叫——這解決了兩個問題：(a) handler 內可呼叫 Subscribe / Clear / Subscription 解構而不死鎖；(b) 防止同步匯流排在派送途中被遞迴 Publish 修改活向量的未定義行為。快照期間被解構的 Subscription 本輪仍觸發，但之後不再。

`Unsubscribe` 以 `unique_lock` 線性掃描找 id 後刪除，early-return。`Clear` 清空全部 handlers，用於 GameController 解構（`~GameController` 呼叫 `bus_.Clear()`），防止訂閱者參照到已釋放的 World。

`Subscription` 是 RAII 移動型別：移動建構子 / 移動賦值以 `std::exchange` 轉移所有權；`Reset()` 先取走所有權（防重入解構），再呼叫 `Unsubscribe(id)` 恰好一次。

## 關鍵內容（類別 / 函式 / 資料）

- `EventBus::Instance()` — Meyer's Singleton；靜態局部變數，執行緒安全初始化（C++11 起）。
- `Subscribe(EventType, Handler)` — 加入永久訂閱；回傳 `*this` 允許鏈式呼叫。
- `ScopedSubscribe(EventType, Handler)` — 加入訂閱並回傳 `Subscription` RAII handle。
- `Unsubscribe(uint64_t id)` — 線性掃描刪除，noexcept。
- `Publish(const Event&)` — shared_lock 快照 → 釋放鎖 → 派送；解決重入與鎖問題。
- `Clear()` — 清空所有 handlers，用於 GameController 解構。
- `Subscription::Reset()` — `std::exchange` 防重入 + `Unsubscribe`；移動建構/賦值正確轉移所有權。

## 相依與在架構中的位置
- **#include（往外）**：`EventBus.h`；`<mutex>`（`unique_lock`）；`<utility>`（`std::exchange`）
- **被誰使用（往內）**：—（葉節點；由 `GameController`、`GameplayScene`、`Harness`、`EventWiring`、`SceneRouter` 等廣泛使用）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / events 層；每幀 `Publish` 在 Controller `Update` 內各系統執行；不直接屬於任一管線階段

## OO 概念與設計重點

此檔案完整體現兩個 GoF 模式：[Singleton](../concepts/pat-singleton.md)（唯一匯流排實例）與 [Observer](../concepts/pat-observer.md)（Subscribe / Publish / Unsubscribe）。`Subscription` RAII 是 [RAII](../concepts/oo-raii.md) 的精典應用：移動語意正確（`std::exchange` 確保所有權唯一轉移）、解構子恰好取消訂閱一次（防重入 `Reset`）。「快照後派送」的執行緒安全策略（`shared_lock` 快照 + 釋放後派送）是在同步匯流排上支援遞迴 Publish 的關鍵設計決策，避免傳統「持鎖派送」的死鎖與迭代器失效問題。`nextId_` 遞增 `uint64_t` 確保 id 不復用，防止延遲解構的 Subscription 誤刪新訂閱者。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/events/EventBus.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/events/EventBus.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Singleton](../concepts/pat-singleton.md) · [Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
