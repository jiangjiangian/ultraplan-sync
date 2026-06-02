---
id: "file:tests/controller/test_eventbus.cpp"
type: test
path: tests/controller/test_eventbus.cpp
domain: tests
bucket: controller
loc: 58
classes: []
sources: ["tests/controller/test_eventbus.cpp"]
---
# `test_eventbus.cpp`

> **一句定位**：驗證全域 `EventBus` 的基礎事件派送語意——正確配送、型別過濾、以及派送途中重入時的迭代安全性。

## 職責

本檔同時擔任 doctest 的 `main`（`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`），是 EventBus 測試套件的進入點。三個 `TEST_CASE` 各自聚焦一個不變式：

**正確派送**：訂閱 `ShowMessage` 後 `Publish` 同型別事件，handler 恰好被呼叫一次，且 `e.text` 正確傳入（`captured == "hello"`）。

**型別過濾**：訂閱 `KarmaChanged`，`Publish` `ShowMessage` 時 handler 不得被呼叫（`hits == 0`），確認匯流排不會廣播給型別不符的訂閱者。

**重入安全（快照派送）**：在 handler 內呼叫 `Subscribe` 再 `Clear`，驗證外層 handler 恰好執行一次（快照機制保護），同時新加入的 handler 在本次派送中不得執行（`reentrant_hits == 0`）。這個 case 釘住了「先對 handler 清單做快照、再迭代快照」的實作決策。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("EventBus 將事件派送給訂閱者")`：基礎配送，驗證 `hits == 1` 且文字正確。
- `TEST_CASE("EventBus 不會派送給型別不符的訂閱者")`：負面 case，`KarmaChanged` 訂閱者不接收 `ShowMessage`。
- `TEST_CASE("EventBus 容許在 handler 內呼叫 Subscribe 與 Clear")`：重入安全；同時驗證 `outer_hits == 1` 且 `reentrant_hits == 0`。

## 相依與在架構中的位置
- **#include（往外）**：`include/engine/events/EventBus.h`（被測主體，含 `Event`、`EventType`、`EventBus::Instance()`）
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純單元測試，無管線角色）

## OO 概念與設計重點

本檔體現了 [Observer](../concepts/pat-observer.md) 模式的核心不變式測試。`EventBus` 是 [Singleton](../concepts/pat-singleton.md)，每個 case 以 `Clear()` 重置全域狀態，實現測試隔離。重入安全 case 直接釘住了「快照再迭代」的設計決策，防止未來的最佳化破壞此保證。測試風格為純 doctest 單元測試，無 GL、無 harness。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_eventbus.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_eventbus.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [Singleton](../concepts/pat-singleton.md)
