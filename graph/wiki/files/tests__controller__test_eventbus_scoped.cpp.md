---
id: "file:tests/controller/test_eventbus_scoped.cpp"
type: test
path: tests/controller/test_eventbus_scoped.cpp
domain: tests
bucket: controller
loc: 161
classes: []
sources: ["tests/controller/test_eventbus_scoped.cpp"]
---
# `test_eventbus_scoped.cpp`

> **一句定位**：驗證 `EventBus::ScopedSubscribe` 回傳的 RAII `Subscription` token 的完整語意：作用域取消訂閱、use-after-free 預防、移動語意所有權轉移，以及與 raw `Subscribe/Clear` 的並存相容性。

## 職責

本檔依賴 `eventbus_isolation` listener 在每個 case 前後自動 `Clear`，因此每個 case 從乾淨的匯流排起步。包含四個 `TEST_CASE`，覆蓋 `EventBus::Subscription` 的核心合約：

**作用域取消訂閱**：`Subscription` 在 `}` 解構後，後續 `Publish` 不再呼叫該 handler（`hits` 停在 1 不再增加）。透過 `sub.Active()` 確認存活狀態。

**use-after-free 預防**：以 `std::make_unique<int>` 建立堆積配置的捕捉物，先 `sub.Reset()` 取消訂閱，再 `captured.reset()` 釋放底層記憶體；之後 `Publish` 時 handler 不得執行（`observed == 1` 不變）。讓 sanitizer 能偵測真正的 UAF。

**移動語意**：三個 subcase：
- 移動建構後原 token `a.Active() == false`、新 token `b.Active() == true`，且 `Publish` 恰好一次觸發。
- 對存活中訂閱做移動指派：`b = std::move(a)` 必須先 Reset b 的舊 handler（`otherHits == 0`），再接管 a 的（`hits == 1`）。
- 銷毀兩個複本只移除 handler 一次：移後原 token 的解構是 no-op，唯一擁有者銷毀後便不再被派送。

**與 raw API 並存**：`raw Subscribe` 與 `ScopedSubscribe` 可同時存在；`ScopedSubscribe` 離開作用域只移除自己的 handler，不影響 raw 訂閱者；`Clear()` 仍可一次清掉全部，且之後對已被 Clear 的 `Subscription` 解構時 `Unsubscribe(id)` 是安全 no-op。

## 關鍵內容（類別 / 函式 / 資料）

- `EventBus::Subscription`（被測型別）：可移動不可複製的 RAII token，解構時精確移除自己的 handler。
- `EventBus::Instance().ScopedSubscribe(type, handler)` → `Subscription`
- `sub.Active()`：查詢 token 是否仍持有存活的訂閱。
- `sub.Reset()`：提早取消訂閱（解構前手動清除）。
- 輔助 `Msg(text)`：建立 `Event{EventType::ShowMessage, text}` 的 factory 函式。

## 相依與在架構中的位置
- **#include（往外）**：`include/engine/events/EventBus.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—

## OO 概念與設計重點

本檔完整測試 [RAII](../concepts/oo-raii.md) 的 `Subscription` token 設計，這是 [Observer](../concepts/pat-observer.md) 模式的重要擴充：從「只有全域 Clear」升級為「精確到 handler 的自動取消訂閱」。移動語意測試確保 token 符合 C++ 移動語義的所有權唯一性保證，且銷毀兩個複本只移除 handler 恰好一次——這是避免 double-unsubscribe bug 的關鍵合約。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_eventbus_scoped.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_eventbus_scoped.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
