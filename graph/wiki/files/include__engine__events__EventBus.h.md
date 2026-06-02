---
id: file:include/engine/events/EventBus.h
type: header
path: include/engine/events/EventBus.h
domain: engine
bucket: events
loc: 144
classes: [Event, EventBus, Subscription, Slot]
sources: ["include/engine/events/EventBus.h"]
---
# `EventBus.h`

> **一句定位**：程序級事件匯流排（Singleton + Observer），讓道具 / UI / 音效透過 Publish / Subscribe / ScopedSubscribe 解耦，並提供 RAII Subscription 權杖避免懸空 handler。

## 職責

`EventBus` 是全域單例，為「道具與遊戲狀態只負責 Publish，UI / 音效等訂閱者各自 Subscribe」的核心機制。發布者和訂閱者雙方不需要互相 `#include`，維持 Model（game 層）與 View / 音效（engine / ui 層）的解耦。

**事件型別** `EventType` 列舉定義了五個事件：`UmbrellaClaimed`（取得雨傘）、`KarmaChanged`（karma 數值變動）、`ShowMessage`（顯示 HUD 訊息）、`EnteredBuilding`（進入建築）、`PickupAcquired`（購買 / 拾取非傘道具）。

**`Event` struct** 攜帶型別 + `text`（文字載荷）+ `slot`（`HudSlot::Top / Bottom`，將 `ShowMessage` 路由到兩條獨立 HUD 頻道，解決章節提示與一般訊息同幀覆蓋問題）。

**`Subscribe(type, handler)`** 安裝永久 handler（至下次 `Clear()`）；**`ScopedSubscribe(type, handler)`** 回傳 `Subscription` RAII 權杖——權杖解構時自動 `Unsubscribe`，是「訂閱生命週期與擁有者範圍嚴格綁定」的機制（避免 handler 捕獲的狀態懸空）。

**`Publish`** 在分派前先快照 handler 清單（`shared_lock` 取快照後即釋放），因此即使在 handler 內部呼叫 `Reset()` 或 `Clear()`，也只影響後續 Publish，不破壞當前遍歷——維持可重入合約。

內部使用 `shared_mutex` 保護 handler 清單，但**不**保護 handler 函式體（handler 可能觸碰 GL context，GL 是單執行緒；切勿從主執行緒以外呼叫 `Publish`）。

## 關鍵內容（類別 / 函式 / 資料）

- **`EventType`**（enum）：`UmbrellaClaimed / KarmaChanged / ShowMessage / EnteredBuilding / PickupAcquired`。
- **`Event`**（struct）：`type`（EventType）、`text`（string，文字載荷 / 道具 id）、`slot`（HudSlot，預設 Bottom）。
- **`EventBus`**（Singleton）：
  - `Instance() → EventBus& static`：取得程序級唯一實例。
  - `Subscribe(EventType, Handler) → EventBus&`：安裝永久 handler，回傳 *this（流暢介面）。
  - `ScopedSubscribe(EventType, Handler) → Subscription`：安裝 RAII 管理 handler，回傳可移動的 Subscription 權杖。
  - `Publish(const Event&) const`：先 shared_lock 取快照再分派，可重入。
  - `Clear() → EventBus&`：移除全部訂閱；`GameController` 解構時呼叫。
  - `Unsubscribe(uint64_t id) noexcept`（私有）：以穩定 id 移除單一 handler。
  - `handlers_`（`unordered_map<EventType, vector<Slot>>`）：型別到 handler 清單的對映。
  - `nextId_`（`uint64_t`）：單調遞增 id 產生器。
  - `mutex_`（`mutable shared_mutex`）：保護 handler 清單的讀寫鎖。
- **`EventBus::Subscription`**（RAII 權杖）：
  - `Active() const noexcept → bool`：是否持有有效訂閱。
  - `Reset() noexcept`：立即移除（冪等）；可在 handler 內安全呼叫。
  - 可移動、不可複製；預設建構或被移走後為 no-op 解構。
- **`Slot`**（私有 struct）：`id`（uint64_t）+ `handler`（Handler）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/events/HudSlot.h`（HUD 頻道列舉）；標準庫 `<functional>`, `<shared_mutex>`, `<unordered_map>` 等。
- **被誰使用（往內）**：被全程式幾乎所有業務層（道具、NPC、Vendor、Quest、State、UI、音訊、Harness、測試）引用——EventBus 是整個系統的事件脊梁。
- **繼承 / 實作 / 體現**：realizes [Observer 模式](../concepts/pat-observer.md)、[RAII / 記憶體安全](../concepts/oo-raii.md)（Subscription 權杖）、[Singleton 模式](../concepts/pat-singleton.md)（`Instance()`）。
- **每幀管線 / MVC 角色**：engine/events 層；不直接參與每幀管線，而是「橫切」整個系統的事件廣播基礎設施。

## OO 概念與設計重點

**[Observer 模式](../concepts/pat-observer.md)**：`EventBus` 是 Subject，訂閱者是 Observer；`Publish` 以快照遍歷解耦「Subject 通知」與「Observer 取消訂閱」的競爭問題。

**[Singleton 模式](../concepts/pat-singleton.md)**：`EventBus::Instance()` 使程序內各處無須傳遞參考即可存取同一個匯流排（但 `EventSink` seam 允許測試替換為區域匯流排，緩解 Singleton 的測試困難）。

**[RAII](../concepts/oo-raii.md)**：`Subscription` 類是 RAII 權杖的典型實現——用 unique ownership + 移動語意取代手動 `Unsubscribe`，徹底消除懸空 handler。`shared_mutex` 快照分派確保可重入（handler 可在分派中安全取消訂閱）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/events/EventBus.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventBus.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md) · [Singleton](../concepts/pat-singleton.md)
