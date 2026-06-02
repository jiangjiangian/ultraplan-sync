---
id: "file:tests/dialog/test_dialog_content_dir.cpp"
type: test
path: tests/dialog/test_dialog_content_dir.cpp
domain: tests
bucket: dialog
loc: 23
classes: [DialogContentDirInit]
sources: ["tests/dialog/test_dialog_content_dir.cpp"]
---
# `test_dialog_content_dir.cpp`

> **一句定位**：測試套件的全域靜態初始化器，在所有 TEST_CASE 開始前將 `DialogSource` 的內容目錄指向正確的對話素材路徑（由建置系統注入 `TEST_CONTENT_DIR`）。

## 職責

本檔不包含任何 `TEST_CASE`，純粹是「一次性全域 fixture」。測試執行檔的 cwd 是 build 目錄，若使用 `DialogSource` 的預設相對路徑（`docs/content`），所有需要讀取 markdown 的測試都會拿到空對話。

`DialogContentDirInit` struct 的 ctor 呼叫 `nccu::dialog::SetContentDir(TEST_CONTENT_DIR)`（由 CMake 等建置系統定義）。`const kDialogContentDirInit`（具有靜態儲存期）在 doctest `main()` 之前完成初始化，因此每個開啟對話的測試都能找到正確的檔案。

這個方法避免了在每個需要對話的 `TEST_CASE` 中手動呼叫 `SetContentDir`（重複且容易遺漏），同時也避免了在執行 binary 時以命令列參數傳遞路徑的複雜性。

## 關鍵內容（類別 / 函式 / 資料）

- `struct DialogContentDirInit`：匿名 namespace 內的 ctor-initializer；ctor 呼叫 `nccu::dialog::SetContentDir(TEST_CONTENT_DIR)`。
- `const kDialogContentDirInit`：靜態儲存期的實例，在程式啟動時（`main` 前）觸發初始化。
- `TEST_CONTENT_DIR`：由建置系統定義的路徑 macro，未定義時編譯失敗（`#error`）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogSource.h`（被設定的全域 content dir）
- **被誰使用（往內）**：—（葉節點 / 組裝根；由 C++ 靜態初始化機制自動呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（測試基礎設施）

## OO 概念與設計重點

本檔體現了「靜態初始化器作為全域 test fixture」的慣用法。其本質是一個極簡的 [RAII](../concepts/oo-raii.md) 初始化模式：把對話路徑設定綁在靜態物件的生命週期，確保所有 case 在執行前都有正確的前置條件。這個設計不需要 doctest 特定的 reporter 機制，比 `EventBusIsolation` 更簡單，因為路徑設定是冪等且全程不變的。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_content_dir.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_content_dir.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
