---
id: file:src/game/vendor/VendorLoader.cpp
type: source
path: src/game/vendor/VendorLoader.cpp
domain: game
bucket: vendor
loc: 219
classes: []
sources: ["src/game/vendor/VendorLoader.cpp"]
---
# `VendorLoader.cpp`

> **一句定位**：把市集 Markdown 內容檔逐行解析為 `VendorConfig` 清單的單趟狀態機，是遊戲所有市集攤位資料的唯一來源。

## 職責

此檔屬於 game / vendor 層，實作 `nccu::vendor::LoadInterludeVendors(const std::string& path)` 函式，以標準 `ifstream` 讀取磁碟上的 Markdown 格式內容檔（UTF-8），透過一趟逐行掃描（單次遍歷）建構並回傳 `std::vector<VendorConfig>`。

解析器由匿名命名空間中的幾個小輔助函式組成，各自解析一類語法結構：
- `ParseStallName(line)` — 識別 `"## 攤位：<name>"` 攤位標題行，取出攤位名稱。
- `ParseSubsectionKey(line)` — 識別 `"### <key>"` 小節行（全形括號前的 token），區分 `greeting`、`onPurchase`/`onDonate`/`onAccept`、`onLeave` 等子節。
- `ParseBulletLine(line, out)` — 解析 `"- \"…\""` 或 `"- "…""` 條列行，提取引號內文字（ASCII 雙引號與 CJK 全形引號皆支援）。
- `ParseField(line, tag, value)` — 解析 `"> <tag>：<value>"` 引言欄位（以全形冒號 `：` 分隔），識別 `攤主`、`商品`（格式 `id=price`）、`機制`、`tier`、`karma`、`stock` 等欄位。

主迴圈以 `flush()` lambda 在每個新攤位標題或檔尾收尾前一個攤位：將 `pendingStock`（延後套用的全局庫存限制）套用到所有品項，並把 `greetingLines[0]` 複製到 `cur.greeting` 作退路。`sub` 狀態機（`None/Greeting/Success/Leave`）追蹤當前子節，確保文字只附加到正確的欄位（`greetingLines`、`onPurchase`、`onLeave`）。`successCaptured` 旗標確保每個攤位只採用第一個成交子節（`onPurchase`/`onDonate`/`onAccept`）。

## 關鍵內容（類別 / 函式 / 資料）

- `LoadInterludeVendors(const std::string& path)` — 公開函式，回傳 `vector<VendorConfig>`；路徑不存在時回傳空向量。
- `ParseStallName`、`ParseSubsectionKey`、`ParseBulletLine`、`ParseField` — 匿名命名空間的四個行解析輔助函式。
- `RStripCr(string&)` — 剝除 Windows 換行的 `\r`，確保 CRLF 檔案可正確解析。
- `Trim(string)` / `StartsWith` / `HasSuffix` — 字串工具，避免依賴外部函式庫。
- `flush()` lambda — 在攤位邊界收尾並把 `cur` 推入 `out`，同時重設所有每攤位的臨時狀態。
- `sub enum class` — `None / Greeting / Success / Leave`，狀態機的子節追蹤器。
- `pendingStock` / `successCaptured` — 每攤位的解析輔助旗標，在 `flush()` 重設。
- `kFullWidthColon`、`kLeftCjkQuote`、`kRightCjkQuote`、`kFullWidthParenL` — UTF-8 字面常數（全形標點），取代手算位元組的易錯做法。

## 相依與在架構中的位置

- **#include（往外）**：`VendorLoader.h`（函式宣告與 `VendorConfig`/`VendorItem` 型別）；標準庫（`fstream`、`string`、`cstdlib`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由市集相關的生成邏輯（`ChapterVendors.h` 的實作）或組裝根在啟動時呼叫，把 Markdown 檔解析後注入 `VendorConfig` 清單。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（非每幀路徑；載入時執行）；屬 Model 層資料讀取，不相依 raylib。

## OO 概念與設計重點

此檔以「簡單狀態機 + Lambda flush」的純手工解析器實現 Markdown 的結構化讀取，完全不相依任何解析函式庫，符合引擎「engine 不反向相依」的架構紅線。UTF-8 全形標點以字面常數而非硬編碼位元組表示，提升可讀性且把易錯點集中在一處。`flush` lambda 把「每攤位收尾」邏輯封裝成閉包，避免重複的初始化代碼出現在兩個地方（遇到新攤位標題時 + 到達檔尾時）。整體風格是純資料解析函式，無類別繼承，體現 SOLID 的 SRP。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/vendor/VendorLoader.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/vendor/VendorLoader.cpp) · [← 全檔索引](../files-index.md)
