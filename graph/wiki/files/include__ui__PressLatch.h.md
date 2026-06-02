---
id: file:include/ui/PressLatch.h
type: header
path: include/ui/PressLatch.h
domain: ui
bucket: 
loc: 48
classes: [PressLatch]
sources: ["include/ui/PressLatch.h"]
---
# `PressLatch.h`

> **一句定位**：跨阻塞式畫面的確認鍵去彈跳閘門——抑制由前一畫面「繼承」而來的同一次按鍵邊緣，直到該鍵被放開過一次，解決單次物理按鍵驅動兩次場景轉場的問題。

## 職責

本標頭解決了一個由 raylib 輸入模型與阻塞式場景迴圈互動引起的具體 bug：raylib 的「pressed」邊緣只在一次輸入輪詢中為 `true`，而唯一的輪詢點是 `EndDrawing`（`DrawScope` 的解構子）。當某個畫面在 Enter 邊緣返回、下一個畫面在同一 `EndDrawing` 之前開始時，新畫面會讀到同一個 Enter 邊緣——結果單次實體按鍵驅動兩次轉場（開啟說明頁立刻又關掉、確認開始遊戲順帶自動選中角色等）。

`PressLatch::Fired(down, pressed)` 以「鍵自上次 arm 以來是否被觀察到放開過一次」作為放行條件。邏輯：
1. `!down`（鍵當前沒按住）→ `armed_ = true`（準備好接受下一次按下）。
2. `armed_ && pressed`（已放開過且偵測到新按下）→ 消耗本次、`armed_ = false`（要求再一次放開才放行）、回傳 `true`。
3. 其他情形回傳 `false`。

新畫面開始時某鍵仍按住（`!down` 為 `false`），故 `armed_` 保持初始的 `false`；第一個 `pressed` 邊緣不被放行——正是「繼承來的按下被忽略」的效果。鍵真正放開後 `armed_` 升為 `true`，下一次按下才被放行。每個畫面對每個確認鍵各持一個 `PressLatch` 實例。

完全不依賴 raylib，是純布林邏輯，可無頭單元測試（`tests/ui/test_press_latch.cpp`）。

## 關鍵內容（類別 / 函式 / 資料）

- `PressLatch`（class）：
  - `Fired(down, pressed) -> bool`（`noexcept`）：核心方法；每幀餵入按鍵位準（`down`）與邊緣（`pressed`），僅在自上次 arm 以來已放開後的第一次新按下回傳 `true`。
  - 私有：`armed_`（`bool`，預設 `false`）——唯有鍵被觀察到放開後才為 `true`。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 include，不依賴 raylib）。
- **被誰使用（往內）**：`include/app/scenes/CharacterSelectScene.h`（選角確認鍵去彈跳）；`include/app/scenes/TitleScene.h`（標題確認鍵去彈跳）；`tests/ui/test_press_latch.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 ui 層的輸入輔助工具。在標題與選角場景的阻塞式迴圈中每幀呼叫 `Fired`，不在主遊戲的每幀管線（Survival→…→Sweep）中出現。

## OO 概念與設計重點

`PressLatch` 是一個微型的 **狀態機**（兩個狀態：`armed=false` 等待放開、`armed=true` 等待按下），以最小的 1-bit 狀態解決了一個具體的輸入事件去彈跳問題。header-only 且無 raylib 依賴，完全可測性是其重要設計屬性——測試能窮舉所有按鍵序列。注解中對 raylib 輸入輪詢時序的詳細說明（`EndDrawing` 是唯一輪詢點）是難得一見的「為何這個 bug 會發生」的完整根因分析。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/PressLatch.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/PressLatch.h) · [← 全檔索引](../files-index.md)
