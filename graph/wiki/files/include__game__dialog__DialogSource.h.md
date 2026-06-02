---
id: "file:include/game/dialog/DialogSource.h"
type: header
path: include/game/dialog/DialogSource.h
domain: game
bucket: dialog
loc: 70
classes: []
sources: ["include/game/dialog/DialogSource.h"]
---
# `DialogSource.h`

> **一句定位**：執行期對話供給的自由函式介面——把 `(NPC id, SemesterState)` 對映到解析子段，透過 `DialogRepository` 接縫讓測試可完全替換內容供應源。

## 職責

`DialogSource.h` 屬 game dialog 層，是「對話內容供給」的公開自由函式介面。它的角色是橋接「呼叫端（`DialogOpener`）」和「底層倉儲（`DialogRepository`）」，同時提供進程全局預設的函式介面，讓所有既有呼叫端無需修改即可使用。

對白供給層設計動機：過去對白內容在建置期凍結進生成標頭，文字微調需重新生成並重編。現改為執行期直接讀取 `docs/content/*.md`，並提供 `Reload()` 讓作者編輯後於下次對話即見效，無需重建。

四個自由函式：
1. **`Entries(npcId, state)`**：委派到當前 `Repository()`（進程預設或測試替換值），回傳子段向量（參考）。未知 npcId/狀態時回傳空 vector，no-throw。
2. **`Reload()`**：清空當前倉儲快取，熱重載鉤子。
3. **`SetContentDir(string dir)`**：覆寫內容目錄，使下次載入走指定路徑。
4. **`SetRepository(DialogRepository* repo)`**：測試替換接縫，傳 `nullptr` 回退進程預設。
5. **`Repository()`**：取當前倉儲（不 null）。

「英文 npcId → 中文 section 名」與「SemesterState → 章節檔名」的固定查表置於 `.cpp`，純查表、跨實例共用但不可變，不造成耦合。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `Entries(npcId, state)` → `const vector<SubEntry>&` | 主要查詢函式；委派 `Repository()`。 |
| `Reload()` | 熱重載快取。 |
| `SetContentDir(string dir)` | 覆寫內容目錄（下次載入生效）。 |
| `SetRepository(DialogRepository*)` | 測試替換接縫；nullptr=回退預設。 |
| `Repository()` | 取當前倉儲。 |
| `nccu::dialog::DialogRepository` | 前向宣告（`SetRepository`/`Repository` 的型別）。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/dialog/DialogLoader.h`（`SubEntry` 型別）、`include/game/state/SemesterState.h`（`SemesterState` 參數型別）；標準庫。
- **被誰使用（往內）**：`src/game/dialog/DialogOpener.cpp`（主要呼叫端）、`src/game/entities/NPC.cpp`（NPC 對話觸發）、大量測試（超過 20 個測試檔，涵蓋各章節測試、Harness 測試、vendor 測試等——是整個對話系統的測試入口）。
- **繼承 / 實作 / 體現**：—（自由函式介面，底層委派 `DialogRepository`）
- **每幀管線 / MVC 角色**：遊戲對話資料層的公開介面；在 `DispatchInteract`→`OpenNpcDialog` 觸發對話時被呼叫。

## OO 概念與設計重點

`DialogSource.h` 採**Façade + Seam**模式：自由函式介面是 Façade（隱藏 `DialogRepository` 實例管理的複雜性），`SetRepository`/`Repository()` 是 Seam（讓測試可替換底層供應源，而不需修改任何呼叫端）。

「進程全局預設倉儲」+「可替換接縫」的組合，是 C++ 中對 Singleton 模式（[pat-singleton](../concepts/pat-singleton.md)）的測試友善版本：正式環境透過全局預設倉儲享受便利性，測試透過接縫享受隔離性，兩者不衝突。

`Entries()` 回傳 `const &`（指向倉儲快取）而非拷貝，是效能優先設計：對白行向量可能很大，避免每次查詢都複製；代價是呼叫端必須在 `Reload()` 之前用完該參考（標頭注解明確記錄了此生命週期約束）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogSource.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogSource.h) · [← 全檔索引](../files-index.md) · 相關概念：[Singleton](../concepts/pat-singleton.md)
