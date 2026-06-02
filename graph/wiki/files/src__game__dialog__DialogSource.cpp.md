---
id: file:src/game/dialog/DialogSource.cpp
type: source
path: src/game/dialog/DialogSource.cpp
domain: game
bucket: dialog
loc: 141
classes: []
sources: ["src/game/dialog/DialogSource.cpp"]
---
# `DialogSource.cpp`

> **一句定位**：對白資料的存取接縫——`DialogRepository` 物件的方法實作，以及進程級「預設倉儲 vs 測試注入倉儲」的切換機制。

## 職責

本檔實作 `DialogRepository` 類別的方法，以及三個自由函式（`Entries`、`Reload`、`SetContentDir`）作為既有 API 的委派入口，維持呼叫端不須感知倉儲物件的存在。

**NPC id 對映**：`NpcNameTable()` 以函式內 `static` 持有一個不可變的 `map<string_view, string>`，將英文 NPC id（`"victim"`、`"suit_senior"`、`"bookworm"` 等）對映到章節 Markdown 中的中文 section 名（`"苦主"`、`"西裝學長"` 等）。涵蓋 Ch1–Ch4 所有主線 NPC、Ch2 圖書館管理員、Ch3 物物交換鏈三節點及 Ch1 氣氛 NPC。

**章節檔對映**：`ChapterFileTable()` 同樣以函式內 `static` 持有 `map<SemesterState, string>`，將學期狀態對映到 `docs/content/` 下的 Markdown 檔名（`chapter1.md` 至 `ending_c.md`）。

**`DialogRepository::ChapterFor(state)`**：惰性載入。先查 `cache_`，命中即返回；未命中則組合 `contentDir_` + 檔名呼叫 `LoadChapter(path)` 載入並插入 cache。未知狀態快取空章節，避免反覆查找。

**`DialogRepository::Entries(npcId, state)`**：先從 `NpcNameTable` 找對應中文名，再從 `ChapterFor(state)` 取 `LoadedChapter`，最後在其 `npcs` map 找對應 NPC 的 `vector<SubEntry>`。任何一步失敗均回傳靜態空 vector。

**進程接縫（`SetRepository` / `Repository`）**：`g_override` 為 nullable 指標，測試可注入自訂 `DialogRepository`（例如指向測試目錄的版本）；遊玩路徑預設使用 `DefaultRepository()`（函式內 `static`，延遲建構，執行緒安全）。

## 關鍵內容（類別 / 函式 / 資料）

- `NpcNameTable() -> const map<string_view,string>&`：英文 id → 中文 section 名的靜態不變表。
- `ChapterFileTable() -> const map<SemesterState,string>&`：學期狀態 → Markdown 檔名的靜態不變表。
- `DefaultRepository() -> DialogRepository&`：進程級預設倉儲，函式內 static 延遲建構。
- `g_override`：`DialogRepository*`，測試注入點。
- `DialogRepository::DialogRepository()`：預設 `contentDir_ = "docs/content"`。
- `DialogRepository::ChapterFor(SemesterState)` → `const LoadedChapter&`：惰性載入，cache 保護。
- `DialogRepository::Entries(npcId, state)` → `const vector<SubEntry>&`：主要查詢介面。
- `DialogRepository::Reload()`：清空 cache 強制重新解析。
- `DialogRepository::SetContentDir(string)`：用於測試環境重設內容目錄。
- `SetRepository(DialogRepository*)` / `Repository() -> DialogRepository&`：進程接縫。
- `Entries(npcId, state)` / `Reload()` / `SetContentDir(string)`：委派到 `Repository()` 的自由函式。

## 相依與在架構中的位置

- **#include（往外）**：`DialogSource.h`（公開宣告）、`DialogRepository.h`（`DialogRepository` / `LoadedChapter` / `SubEntry` 型別及 `LoadChapter`）；標準庫 `<map>` / `<string>` / `<string_view>` / `<vector>`。
- **被誰使用（往內）**：`DialogOpener.cpp`（`nccu::dialog::Entries` 查詢子段）、`NPC.cpp`（`NPC::LoadDialog` 查詢子段）。
- **繼承 / 實作 / 體現**：—（方法實作檔）。
- **每幀管線 / MVC 角色**：Model 層資料存取。在 Controller 互動觸發時（每幀管線的「E 互動」階段）間接被 `DialogOpener` 查詢，本身不在管線中直接執行。

## OO 概念與設計重點

本檔體現了 [Singleton 模式](../concepts/pat-singleton.md) 的一種變體：`DefaultRepository()` 以函式內 `static` 持有唯一的預設倉儲，避免全域物件的靜態初始化順序問題；同時 `g_override` 允許測試在不破壞真正 Singleton 不變式的前提下替換倉儲。`NpcNameTable` / `ChapterFileTable` 同樣以函式內 `static const` 持有，是不可變純資料的最輕量共用方案，跨 `DialogRepository` 實例無需額外同步。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogSource.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogSource.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Singleton](../concepts/pat-singleton.md)
