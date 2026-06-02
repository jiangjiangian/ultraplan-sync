---
id: "file:tests/ui/test_large_targets.cpp"
type: test
path: tests/ui/test_large_targets.cpp
domain: tests
bucket: ui
loc: 104
classes: []
sources: ["tests/ui/test_large_targets.cpp"]
---
# `test_large_targets.cpp`

> **一句定位**：驗證「擴大目標」無障礙旗標的預設值、setter 雙向切換、與減少動畫的獨立性，以及環境變數 `UMBRELLA_LARGE_TARGETS` 的接線。

## 職責

本測試固定 `World::LargeTargets()` 旗標的三個面向：

1. **預設為 false**：清除 `UMBRELLA_LARGE_TARGETS` 環境變數後建立 `World`，確認旗標預設關閉，以維持既有 8px 互動距離行為的確定性。

2. **setter 雙向切換**：`SetLargeTargets(true)` 開啟、`SetLargeTargets(false)` 關回，確認不是像 `kFlagHasTrueUmbrella` 那樣的單向鎖存。同時驗證與 `ReducedMotion` 旗標完全獨立（切換其中一個不影響另一個）。

3. **環境變數接線**：`UMBRELLA_LARGE_TARGETS=1` 使 `ReadWorldOptionsFromEnv()` 解析出 `opts.largeTargets=true`，World 建構子遵循其結果；`"0"` 或未設定維持 false（只有字面 `"1"` 才啟用）。

旗標所代表的實際語意（E 互動距離放寬到 16px，有效對話框 56x56 → 40x40）不在此測試（由 `GameController::Update` 依同一旗標讀取）；測試只驗證旗標狀態本身。

## 關鍵內容（類別 / 函式 / 資料）

- `World::LargeTargets()` / `World::SetLargeTargets(bool)` — 被測。
- `World::ReducedMotion()` / `World::SetReducedMotion(bool)` — 用於驗證獨立性。
- `nccu::ReadWorldOptionsFromEnv()` — 被測：解析環境變數為 `WorldOptions` 結構。
- `nccu::WorldOptions` — 含 `largeTargets` 欄位。
- `setenv("UMBRELLA_LARGE_TARGETS", "1", 1)` / `unsetenv(...)` — 測試 fixture 清理環境。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/World.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試（`World` 旗標），旗標被 Controller 的 `GameController::Update` 消費。

## OO 概念與設計重點

純 doctest 單元測試，以 `unsetenv`/`setenv` 控制測試 fixture。獨立性測試（兩個無障礙旗標互不干擾）體現了「無交叉耦合（no cross-coupling）」的設計約束，防止日後重構把兩個旗標合一。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_large_targets.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_large_targets.cpp) · [← 全檔索引](../files-index.md)
