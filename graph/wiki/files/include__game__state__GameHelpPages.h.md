---
id: file:include/game/state/GameHelpPages.h
type: header
path: include/game/state/GameHelpPages.h
domain: game
bucket: state
loc: 25
classes: []
sources: ["include/game/state/GameHelpPages.h"]
---
# `GameHelpPages.h`

> **一句定位**：遊戲說明覆蓋層總頁數 `kGameHelpPageCount = 2` 的 game 層定義，斷開 game→ui 的反向相依。

## 職責

`GameHelpPages.h` 是一個極度精簡的常數標頭，僅定義 `inline constexpr int kGameHelpPageCount = 2`，供需要翻頁的 game 層場景與控制器使用（標題場景、暫停畫面）。

設計動機與 `InventoryPaging.h` 完全相同：`ui/GameHelp.h` 定義實際頁面內容及其陣列大小，但 game 層控制器（`PauseScreen`、標題場景）需要知道頁數以限制翻頁游標。若直接引用 `ui/GameHelp.h`，會產生 game→ui 的非法反向相依。解決方案是在 game 層此標頭中刻意重複這個小常數，而由 `ui/GameHelp.h` 一端的 `static_assert` 確保兩者保持一致。

## 關鍵內容（類別 / 函式 / 資料）

- `kGameHelpPageCount`（`inline constexpr int = 2`）：遊戲說明覆蓋層的總頁數，供翻頁游標邊界判定使用。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 #include）
- **被誰使用（往內）**：`include/ui/GameHelp.h`（另含 `static_assert` 釘住一致性）、`src/game/controller/GameController.cpp`、`src/game/controller/screens/PauseScreen.cpp`（翻頁邊界判定）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純常數定義；Controller 讀取以處理翻頁輸入邊界，View 端保有相同值並以 static_assert 驗證。

## OO 概念與設計重點

與 `InventoryPaging.h` 相同的「架構守衛常數」模式：以刻意重複一個小常數的代價，換取 game→ui 的依賴邊界完整。`static_assert` 是確保重複常數不漂移的靜態驗證工具，比執行期比對或由人工維護更可靠。注釋中明確說明「未來新增頁面時須同時更新兩處，由 static_assert 把關」，體現了**最小可行架構守衛**的實用哲學。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/GameHelpPages.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/GameHelpPages.h) · [← 全檔索引](../files-index.md)
