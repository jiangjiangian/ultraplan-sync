---
id: "file:tests/dialog/test_dialog_repository.cpp"
type: test
path: tests/dialog/test_dialog_repository.cpp
domain: tests
bucket: dialog
loc: 126
classes: []
sources: ["tests/dialog/test_dialog_repository.cpp"]
---
# `test_dialog_repository.cpp`

> **一句定位**：驗證 `DialogRepository` 的可注入設計——`SetRepository(nullptr)` 走全域預設實例、`SetRepository(&repo)` 改導 `Entries()` 到指定實例、不同實例的快取真正隔離，且對某實例 `Reload()` 不影響其他實例。

## 職責

`DialogRepository` 是為了取代 `DialogSource.cpp` 內原本的檔案靜態可變狀態而設計的可注入快取層。本檔包含 4 個 `TEST_CASE`，驗證其依賴注入接縫的三個不變式：

**預設路徑向後相容**：`SetRepository(nullptr)` 確保 `Entries()` 仍走全域預設實例（向後相容），Ch1 助教區段不為空（`CHECK_FALSE(subs.empty())`）。每個 case 開頭都呼叫 `SetRepository(nullptr)` 從乾淨狀態起步，防止跨 case 狀態外溢。

**注入實例接管 `Entries()`**：建立 `myRepo`，設定相同的 `TEST_CONTENT_DIR`，呼叫 `SetRepository(&myRepo)` 後 `Repository() == &myRepo`；透過自由函式 `Entries()` 與直接呼叫 `myRepo.Entries()` 取得的向量位址相同（同一實例，同一 map 節點）。測試後還原 `SetRepository(nullptr)`。

**快取隔離**：兩個實例（一個指向真實 fixture、一個指向不存在目錄 `/no/such/dir/p25-isolation`）的快取不外洩：fixture repo 的 `Entries` 非空，空目錄 repo 的 `Entries` 為空；再次查詢 fixture repo 的向量位址不變（map 節點穩定），且仍非空。

**跨實例 `Reload()` 隔離**：兩個指向同一內容目錄的 `repoA/repoB`，只 `repoB.Reload()`；`repoA` 的快取參考在 reload 後仍存活（`&aAfter == &aBefore`）；`repoB` 下次查詢時重新從磁碟讀取（回傳非空 vector）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::dialog::DialogRepository`（被測型別）：可注入的對話快取，每個實例有獨立的 content dir 與 map 快取。
- `nccu::dialog::SetRepository(DialogRepository*)`：注入覆寫（nullptr 還原預設）。
- `nccu::dialog::Repository()`：取得目前作用中的 repository 實例（參考）。
- `nccu::dialog::Entries(npcId, SemesterState)`：自由函式接縫，委派到 `Repository().Entries()`。
- `DialogRepository::SetContentDir(path)`、`DialogRepository::Entries(npcId, SemesterState)`、`DialogRepository::Reload()`。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogRepository.h`、`include/game/dialog/DialogSource.h`、`include/game/state/SemesterState.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料層單元測試）

## OO 概念與設計重點

本檔測試的是一個典型的「Service Locator / 依賴注入接縫」重構：把全域靜態可變狀態（原 `DialogSource.cpp` 的 file-static cache）包裝成可注入的 `DialogRepository`，使測試能建立隔離的實例而不影響正式執行路徑。快取隔離 case（不同 content dir）和 Reload 隔離 case 共同確認這個重構是「真正解耦」而非僅換名字。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_repository.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_repository.cpp) · [← 全檔索引](../files-index.md)
