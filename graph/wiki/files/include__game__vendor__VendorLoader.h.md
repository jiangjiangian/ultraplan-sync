---
id: file:include/game/vendor/VendorLoader.h
type: header
path: include/game/vendor/VendorLoader.h
domain: game
bucket: vendor
loc: 40
classes: []
sources: ["include/game/vendor/VendorLoader.h"]
---
# `VendorLoader.h`

> **一句定位**：宣告 `LoadInterludeVendors(path)` 函式，是從幕間市集 Markdown 檔案解析 `VendorConfig` 清單的執行期解析器——`dialog::LoadChapter` 的「價目表姊妹版」。

## 職責

`VendorLoader.h` 在 `nccu::vendor` 命名空間中宣告單一函式 `LoadInterludeVendors(const string& path)`，解析幕間市集內容檔（`interlude_market.md`）以產生 `VendorConfig` 向量。

設計動機明確：取代「手工抄寫十個字面常數」的舊作法（此為本專案刻意淘汰的 codegen 模式），使市集內容以 Markdown 為單一真實來源，改 .md 即改遊戲，無須重新編譯。區段格式與章節對話完全平行：`## 攤位：<攤名>`、`> 攤主：<人>`、`> 商品：<itemId> = <price>`（0..n 行）、`> 機制：<buy|donate|...>`、`> tier：<N>`、`> karma：<±N>`、`> stock：<N>`、`### greeting`（底下 `- "…"` 台詞行）、`### onPurchase`/`### onLeave`。

帶變體後綴的區塊（如 `### onPurchase（陷阱傘殘骸）`）會被解析，但目前只保留第一個成交區塊。台詞行格式與章節對話相同（`- "…"` ASCII 或全形引號）。開檔失敗時回傳空向量（不丟例外），與 `LoadChapter` 的「降級為空」契約一致，讓遊戲在缺少內容檔時仍能啟動（只是市集無攤位）。

## 關鍵內容（類別 / 函式 / 資料）

- `LoadInterludeVendors(const string& path) → vector<VendorConfig>`：解析幕間市集 .md 文件，回傳 VendorConfig 清單；開檔失敗回傳空向量。
- 命名空間 `nccu::vendor`：與 `nccu` 其他 game 層子模組並行但保持獨立命名空間。

## 相依與在架構中的位置

- **#include（往外）**：`VendorConfig.h`（回傳型別）；標準庫 `<string>`、`<vector>`
- **被誰使用（往內）**：`src/game/quest/ChapterVendors.cpp`（`ChapterVendors(Interlude_Market)` 首次呼叫時解析）、`src/game/vendor/VendorLoader.cpp`（實作體）、`tests/vendor/test_vendor_loader.cpp`（解析器單元測試）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：建構期工具——在幕間市集首次被請求時（`ChapterVendors(Interlude_Market)` 第一次呼叫）解析一次並快取，後續不再重新解析；熱重載時由 `ReloadVendors()` 觸發重解析。

## OO 概念與設計重點

本設計是**內容驅動開發（Content-Driven Design）**的一部分：遊戲內容（市集攤位、商品、台詞）儲存在作者可直接編輯的 Markdown 文件，執行期解析器作為橋接層，讓 Markdown 變成 VendorConfig 物件圖。這與 `dialog::LoadChapter` 對話系統完全平行，降低企劃的技術門檻（不需修改 C++ 即可調整市集內容）。「降級為空」的錯誤處理契約（開檔失敗回傳空向量）是防禦性設計，與整個內容系統的一致錯誤策略保持一致。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/vendor/VendorLoader.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorLoader.h) · [← 全檔索引](../files-index.md)
