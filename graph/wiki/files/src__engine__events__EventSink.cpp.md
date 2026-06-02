---
id: "file:src/engine/events/EventSink.cpp"
type: source
path: src/engine/events/EventSink.cpp
domain: engine
bucket: events
loc: 21
classes: []
sources: ["src/engine/events/EventSink.cpp"]
---
# `EventSink.cpp`

> **一句定位**：實體層發布接縫：以行程層級指標 `g_sink` 讓各編譯單元的 `Publish` 可被導向自訂 `EventBus`，預設退回 `EventBus::Instance()`。

## 職責

`EventSink.cpp` 管理一個行程層級的靜態指標 `g_sink`（初始 `nullptr`），並提供兩個函式：

`SetSink(EventBus*)` — 將 `g_sink` 設為給定指標；傳入 `nullptr` 重置。`GameplayScene` 在建構後呼叫 `SetSink(&EventBus::Instance())`；在 `Exit()` 呼叫 `SetSink(nullptr)` 以便重啟循環乾淨重新綁定。`main.cpp` 的 `Run` 回傳後也呼叫 `SetSink(nullptr)` 卸下接縫。

`Sink()` — 回傳 `g_sink ? *g_sink : EventBus::Instance()`：若已設 sink 則使用注入的匯流排，否則退回全域 Singleton。此設計使「只建構 Player 的測試」無須任何初始化即可使用 `EventBus::Instance()`，行為與導入注入式 sink 之前完全一致（byte 級相同）。

此模組實現「發布端不直接依賴 Singleton」的依賴倒置：實體與任務程式碼呼叫 `nccu::events::Sink().Publish(...)` 而非 `EventBus::Instance().Publish(...)`，由此統一切換點。

## 關鍵內容（類別 / 函式 / 資料）

- `g_sink` — 匿名命名空間的 `EventBus*`，行程層級，初始 nullptr。
- `SetSink(EventBus* bus) noexcept` — 設定或重置 sink 指標。
- `Sink() noexcept -> EventBus&` — 條件性回傳注入匯流排或 `EventBus::Instance()`。

## 相依與在架構中的位置
- **#include（往外）**：`EventSink.h`（唯一直接依賴）
- **被誰使用（往內）**：—（由 `GameplayScene`、`main.cpp`、各 entity/quest 模組透過 `Sink()` 使用）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / events 層；提供發布接縫，不在模擬管線中執行，但每幀的 entity Publish 呼叫都通過 `Sink()`

## OO 概念與設計重點

`EventSink` 是「可測試性接縫」的典型設計：透過行程層級指標讓測試程式碼能以 spy / mock `EventBus` 替換真實匯流排，而不修改發布端程式碼。這是 DIP（依賴倒置）的簡化實現——全域函式替代了介面注入，但提供了同等的可替換性。`Sink()` 的退回邏輯（`nullptr` → `Instance()`）確保現有測試無須改動即可繼續工作，體現向後相容設計原則。與 `EventBus` 的 [Singleton](../concepts/pat-singleton.md) + [Observer](../concepts/pat-observer.md) 共同構成整個事件系統。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/events/EventSink.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/events/EventSink.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Singleton](../concepts/pat-singleton.md) · [Observer](../concepts/pat-observer.md)
