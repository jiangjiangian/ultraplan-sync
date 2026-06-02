---
id: "file:include/game/dialog/DialogRepository.h"
type: header
path: include/game/dialog/DialogRepository.h
domain: game
bucket: dialog
loc: 70
classes: [DialogRepository]
sources: ["include/game/dialog/DialogRepository.h"]
---
# `DialogRepository.h`

> **一句定位**：對話內容的「實例化倉儲」——把原本檔案靜態可變的章節快取與內容目錄收斂成實例範圍物件，讓測試可建構私有倉儲達成完整隔離。

## 職責

`DialogRepository` 屬 game dialog 層，是對話資料供給的「可替換實例」。它是對 `DialogSource.h` 自由函式介面的底層支撐——把快取和目錄設定從進程全局靜態可變狀態轉移到物件實例，解決了「多測試案例共用同一快取會互相污染」的問題。

核心設計：每個 `DialogRepository` 實例持有：
- `contentDir_`：章節 Markdown 所在目錄（預設 `"docs/content"`）
- `cache_`：`map<SemesterState, LoadedChapter>`，每個學期狀態各一份快取（惰性載入）

**三個公開方法**：
- `Entries(npcId, state)`：取某 NPC 在某狀態章節的子段向量。若章節未快取則呼叫 `ChapterFor(state)` 惰性載入並快取。回傳的參考在「本實例」下次 `Reload()` 前有效（`std::map` 節點穩定）。
- `Reload()`：清空快取，使下次 `Entries()` 重新自磁碟讀取——熱重載鉤子，測試案例間隔離用。
- `SetContentDir(string dir)`：覆寫內容目錄，於下次載入時生效——讓測試指向固定測試內容目錄。

`DialogRepository` 本身是「可實例化的服務物件」；`DialogSource.h` 的 `SetRepository()`/`Repository()` 接縫讓測試替換進程當前倉儲，舊呼叫端維持不變。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class DialogRepository` | 持有 `contentDir_` 和 `cache_`（`map<SemesterState, LoadedChapter>`）的服務物件。 |
| `DialogRepository()` | 建構子；初始化預設 `contentDir_="docs/content"`。 |
| `Entries(npcId, state)` → `const vector<SubEntry>&` | 惰性載入章節後查表；參考在本次 `Reload()` 前有效。 |
| `Reload()` | 清空快取；熱重載/測試隔離。 |
| `SetContentDir(string dir)` | 覆寫內容目錄。 |
| `ContentDir()` | 讀取目前目錄。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/dialog/DialogLoader.h`（`LoadedChapter`/`SubEntry`/`LoadChapter`）、`include/game/state/SemesterState.h`（快取鍵型別）；標準庫 `<map>`/`<string>`/`<string_view>`/`<vector>`。
- **被誰使用（往內）**：`src/game/dialog/DialogSource.cpp`（持有進程預設實例，`Repository()` 回傳它）、`tests/dialog/test_dialog_repository.cpp`（建構私有 `DialogRepository` 隔離測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：遊戲對話資料層的倉儲；不參與每幀管線，在對話路由（`DialogOpener`→`DialogSource`→`DialogRepository`）時按需載入。

## OO 概念與設計重點

`DialogRepository` 體現了**Dependency Injection + Testability**設計：把原本難以替換的進程全局快取封裝為可實例化的物件，測試可建構私有實例而不影響其他測試。這是**Repository 模式**（在 DDD 中用於隔離資料存取）的輕量應用，把「從磁碟讀取並快取對話資料」的職責聚焦在單一類別中。

`map<SemesterState, LoadedChapter>` 以 FSM 狀態為鍵的設計，天然地將「不同章節的資料不會混淆」作為型別系統保證，比以字串為鍵更健壯。`std::map` 的節點穩定性（插入不使現有參考失效）允許 `Entries()` 安全地回傳 `const &`，避免不必要的拷貝。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogRepository.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogRepository.h) · [← 全檔索引](../files-index.md)
