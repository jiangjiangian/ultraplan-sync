---
id: "file:tests/controller/test_eventbus_isolation.cpp"
type: test
path: tests/controller/test_eventbus_isolation.cpp
domain: tests
bucket: controller
loc: 43
classes: [EventBusIsolation]
sources: ["tests/controller/test_eventbus_isolation.cpp"]
---
# `test_eventbus_isolation.cpp`

> **一句定位**：在 doctest 每個 test case 與 subcase 的邊界自動 `Clear` 全域 `EventBus`，使整個測試套件對 EventBus 狀態保持 hermetic 隔離。

## 職責

本檔不包含任何 `TEST_CASE`，純粹扮演「測試基礎設施」角色。它實作了一個 doctest `IReporter`（`EventBusIsolation`），並以 `REGISTER_LISTENER` 向框架登記，使其鉤入每個 test case 與 subcase 的生命週期。

背景問題：`EventBus` 沒有逐 handler 的取消訂閱機制，只有全域 `Clear()`。許多測試訂閱捕捉了堆疊變數或 `this` 的 lambda，若跨 case 遺留，下次 `Publish` 時會對已銷毀的捕捉物呼叫 handler，導致 use-after-free 並中止整個執行。正式程式不受影響（只有 `GameController` 訂閱，其解構負責 `Clear`），但測試套件需要此守門者。

`EventBusIsolation` 在以下六個時機呼叫 `Reset()`（即 `EventBus::Instance().Clear()`）：
- `test_case_start`、`test_case_reenter`、`test_case_end`
- `subcase_start`、`subcase_end`

其餘鉤子（`report_query`、`test_run_start`、`test_run_end` 等）皆為 no-op。

## 關鍵內容（類別 / 函式 / 資料）

- `struct EventBusIsolation : doctest::IReporter`：doctest reporter 的實作，管理 EventBus 的 per-case 清理。
- `static void Reset()`：呼叫 `EventBus::Instance().Clear()`，作為各鉤子的共用工具函式。
- `REGISTER_LISTENER("eventbus_isolation", 1, EventBusIsolation)`：以優先序 1 登記 listener，確保在所有編譯單元的測試執行前都已生效。

## 相依與在架構中的位置
- **#include（往外）**：`include/engine/events/EventBus.h`（被清理的主體）
- **被誰使用（往內）**：—（葉節點 / 組裝根；doctest 框架以反射機制透過 `REGISTER_LISTENER` 呼叫）
- **繼承 / 實作 / 體現**：`EventBusIsolation` 繼承 `doctest::IReporter`
- **每幀管線 / MVC 角色**：—（測試基礎設施，無管線角色）

## OO 概念與設計重點

本檔體現了 [Observer](../concepts/pat-observer.md) 模式的測試可靠性問題，並以框架鉤子（doctest reporter 介面）解決。其本身是一種「測試 fixture 即 RAII」的思路：隔離邊界由框架保證觸發，而非各 case 手動呼叫 `Clear`，完全消除了人為遺漏的風險。[RAII](../concepts/oo-raii.md) 精神：每個 case 的資源（匯流排狀態）在邊界被自動釋放。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_eventbus_isolation.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_eventbus_isolation.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md)
