---
id: file:include/engine/platform/WorkingDir.h
type: header
path: include/engine/platform/WorkingDir.h
domain: engine
bucket: platform
loc: 39
classes: []
sources: ["include/engine/platform/WorkingDir.h"]
---
# `WorkingDir.h`

> **一句定位**：工作目錄正規化的 header-only 工具，確保遊戲相對資源路徑（resources/、docs/content/）可解析，防止從 IDE / 點兩下 / build 目錄啟動時的資源載入失敗。

## 職責

此標頭定義單一 `inline` 函式 `nccu::engine::platform::EnsureAssetWorkingDir()`，解決一個具體的部署問題：遊戲的所有資源（紋理、字型、地圖、對話檔）以相對路徑 `resources/...` 和 `docs/content/...` 載入，從專案根目錄執行正常，但從 IDE / 檔案管理員 / 點兩下 / `build/` 目錄執行時 CWD 錯誤，導致資源載入失敗。

解法：檢查 CWD 是否同時含 `resources/` 和 `docs/content/` 兩個目錄（`::DirectoryExists`）：
- **是**：直接回傳（幂等，不改動 CWD）——從專案根執行、ctest、自動遊玩皆走此路徑。
- **否**：切換到 `::GetApplicationDirectory()`（執行檔所在目錄，CMake 會把兩個資料夾複製到這裡）。

背景說明（程式碼內的詳細注解）：macOS 上 raylib 停用了 GLFW 的 `chdir-to-bundle`；缺少 `docs/content` 曾導致字型載入走向過大圖集的退路，在部分 GPU 上造成啟動崩潰（`GL_MAX_TEXTURE_SIZE` 溢出）。此函式須在啟動時、第一次呼叫 `EnsureFont` 之前呼叫一次。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::platform::EnsureAssetWorkingDir() → void`**（`inline`）：
  - 呼叫 `::DirectoryExists("resources") && ::DirectoryExists("docs/content")` 判斷 CWD。
  - 若不符合，呼叫 `::ChangeDirectory(::GetApplicationDirectory())` 切換到執行檔目錄。
  - 具冪等性：可安全多次呼叫。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`::DirectoryExists / ::GetApplicationDirectory / ::ChangeDirectory`）——raylib 使用限制在引擎層。
- **被誰使用（往內）**：`src/app/main.cpp`（啟動時呼叫一次，在 `InitWindow` 和 `EnsureFont` 之前）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/platform 層；程式啟動時一次性呼叫，不參與每幀管線。

## OO 概念與設計重點

Header-only `inline` 工具函式——最小設計，無類別、無狀態、無副作用（對已正確的 CWD 為 no-op）。把 `chdir` 這個有副作用的平台操作封裝在引擎層，讓 `main.cpp`（app 層）保持無需知道 raylib 目錄 API 的細節，符合「raylib 限制在引擎層」的架構規則。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/WorkingDir.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/WorkingDir.h) · [← 全檔索引](../files-index.md)
