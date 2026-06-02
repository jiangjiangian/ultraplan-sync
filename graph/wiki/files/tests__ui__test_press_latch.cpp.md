---
id: "file:tests/ui/test_press_latch.cpp"
type: test
path: tests/ui/test_press_latch.cpp
domain: tests
bucket: ui
loc: 74
classes: []
sources: ["tests/ui/test_press_latch.cpp"]
---
# `test_press_latch.cpp`

> **一句定位**：驗證 `PressLatch` 的「前一畫面延續的按鍵必須先釋放再武裝才能觸發」不變量，防止畫面切換時同一個 Enter 觸發兩次。

## 職責

`PressLatch` 解決的問題：raylib 的「剛按下」邊緣訊號會持續到下一次輪詢，當畫面 A 在 Enter 上返回、畫面 B 在輪詢前開始，同一個 Enter 會觸發兩次（說明畫面開了又關；選角畫面第 1 幀自動確認角色）。

`PressLatch` 的修正語意：建立時若按鍵已按下，忽略任何按下，直到觀察到一次釋放為止（「武裝」）；武裝後第一次新的 `IsPressed` 才觸發 `Fired()` 為 true，且按住期間不自動重複。

測試案例涵蓋：
1. **從前一畫面延續按住**：建立時已按下，不論按住多久都不觸發；釋放後武裝；下一次真正的新按下觸發一次；按住中不再重複。
2. **一開始就是釋放狀態**：第 1 幀釋放 → 武裝；第一次 `IsPressed` 立即觸發。
3. **「標題 → 說明 → 標題」來回情境**：兩個 `PressLatch` 實例，每個都有自己的武裝/觸發狀態，模擬完整的 Enter 傳遞與抑制流程。
4. **「標題開始遊戲 → 選角」交接情境**：選角的 `PressLatch` 吞掉繼承的 Enter，使玩家必須再次按下才能確認角色。

所有測試都是純邏輯，無視窗/GL 需求，可在無頭環境執行。

## 關鍵內容（類別 / 函式 / 資料）

- `PressLatch::Fired(bool down, bool pressed)` — 被測：回傳是否應視為一次有效的確認觸發。
- `PressLatch`（狀態）：武裝（armed）/ 未武裝兩種內部狀態，依 `down` 和 `pressed` 的組合轉換。

## 相依與在架構中的位置

- **#include（往外）**：`ui/PressLatch.h`（受測主體）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（UI 工具類測試，不在每幀管線中）

## OO 概念與設計重點

純 doctest 單元測試，對一個微小的狀態機進行白盒測試。情境驅動的測試案例（標題↔說明↔選角）提供了比抽象規格更直接的回歸防護，清楚表達了「同一個 Enter 最多觸發一次」的使用者體驗需求。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_press_latch.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_press_latch.cpp) · [← 全檔索引](../files-index.md)
