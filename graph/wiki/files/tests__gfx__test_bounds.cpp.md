---
id: file:tests/gfx/test_bounds.cpp
type: test
path: tests/gfx/test_bounds.cpp
domain: tests
bucket: gfx
loc: 40
classes: []
sources: ["tests/gfx/test_bounds.cpp"]
---
# `test_bounds.cpp`

> **一句定位**：驗證 `ClampToWorld` 的邊界夾限語意——點在範圍內不動、超出左上角夾至 0、超出右下角保留整個尺寸在範圍內、尺寸大於世界時軸向夾至 0。

## 職責

此測試以四個 TEST_CASE 完整覆蓋 `ClampToWorld()` 的所有邊界情況。該函式接受位置、物件尺寸與世界大小三個 `Vec2`，回傳夾限後的位置，確保物件整體（含尺寸）不超出世界邊界。這是移動系統或相機系統呼叫的純工具函式，屬 `tests/gfx` 層的數學邏輯單元測試。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ClampToWorld：點落在世界內時位置不變")`：輸入 `(1000,1000)`，尺寸 `(24,24)`，世界 `(2048,2048)`；斷言輸出不變。
- `TEST_CASE("ClampToWorld：超出左上角時夾限至 0")`：輸入 `(-50,-10)`；斷言兩軸均夾至 0。
- `TEST_CASE("ClampToWorld：超出右下角時夾限使整個尺寸仍留在範圍內")`：輸入 `(3000,3000)`；斷言夾至 `(2048-24, 2048-24)` = `(2024, 2024)`。
- `TEST_CASE("ClampToWorld：尺寸大於世界時該軸位置夾限至 0")`：尺寸 x=4096 > 世界 x=2048；斷言 x 夾至 0，y 不受影響。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Vec2.h`，`game/gfx/Bounds.h`（`ClampToWorld` 定義）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純數學工具測試）

## OO 概念與設計重點

純資料邏輯的 doctest 單元測試，無狀態依賴。`ClampToWorld` 是無副作用的純函式，測試以邊界值分析（左上、中央、右下、超尺寸）覆蓋所有分支，是 header-only 工具函式測試的典型模式。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_bounds.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_bounds.cpp) · [← 全檔索引](../files-index.md)
