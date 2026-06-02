---
id: "file:tests/vendor/test_vendor_loader.cpp"
type: test
path: tests/vendor/test_vendor_loader.cpp
domain: tests
bucket: vendor
loc: 115
classes: []
sources: ["tests/vendor/test_vendor_loader.cpp"]
---
# `test_vendor_loader.cpp`

> **一句定位**：驗證 `LoadInterludeVendors` 從真實內容檔（`interlude_market.md`）正確解析十個攤位的完整資料——商品、業力、有限庫存、多成功區塊的取捨，以及 `ChapterVendors` 的快取行為。

## 職責

本測試對真實內容檔進行整合測試，確保內容作者破壞格式時能立即被攔下。需要建置系統定義 `TEST_CONTENT_DIR`。

**解析全部十攤**：`LoadInterludeVendors(path)` 回傳 size=10；攤位 0（熱騰騰雞排攤）的完整資料驗證：
- `name = "熱騰騰雞排攤"`、`stallKeeper = "炸物阿姨"`、`mechanic = "buy"`、`tier = 1`。
- 一個商品（`HotPack`，25 元，`stockLeft = -1` 無限）。
- 3 行問候（`greetingLines.size() == 3`），`greeting` 等於第 0 行。
- `onPurchase.size() == 2`、`onLeave.size() == 1`。

**募款箱（學生會募款箱）**：`mechanic = "donate"`、`karmaOnInteract = 1`；一個商品（Donation，10 元，`stockLeft = 5`）。

**多成功區塊（畢業生二手書攤）**：`mechanic = "sell"`；`stock.empty()`（無商品行）；`onPurchase.size() == 2`，第 0 項為「這把傘骨架怎麼這樣……算了，我拆材料用。」（第一個成功區塊生效，其他丟棄）。

**`ChapterVendors` 快取**：一般章節（Ch1）回傳空列表；`Interlude_Market` 回傳 10 攤；第二次呼叫回傳同一份底層儲存（指標相等）。所有攤位 Y < 1900.0f（遠在南側出口以北），且十個位置兩兩相異。

## 關鍵內容（類別 / 函式 / 資料）

- `vendor::LoadInterludeVendors(path)` — 被測主要解析函式，回傳 `std::vector<VendorConfig>`。
- `Find(vector<VendorConfig>&, name)` — 輔助搜尋函式。
- `nccu::ChapterVendors(SemesterState)` — 被測快取函式。
- `nccu::SetVendorContentDir(dir)` / `nccu::ReloadVendors()` — fixture 設定。
- `VendorConfig` 欄位：`name`、`stallKeeper`、`mechanic`、`tier`、`stock`（含 `itemId`/`price`/`stockLeft`）、`karmaOnInteract`、`greetingLines`、`greeting`、`onPurchase`、`onLeave`。

## 相依與在架構中的位置

- **#include（往外）**：`game/vendor/VendorLoader.h`（受測主體）、`game/quest/ChapterVendors.h`、`game/state/SemesterState.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（World 初始化前的內容載入層測試）

## OO 概念與設計重點

對真實內容檔的整合測試，讓「內容即規格」：任何作者編輯 `interlude_market.md` 破壞了解析器預期的格式，此測試就會失敗。快取指標相等測試（`&m == &m2`）確認快取語意而非深度拷貝。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/vendor/test_vendor_loader.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/vendor/test_vendor_loader.cpp) · [← 全檔索引](../files-index.md)
