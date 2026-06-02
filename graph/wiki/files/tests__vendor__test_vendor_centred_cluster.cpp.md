---
id: "file:tests/vendor/test_vendor_centred_cluster.cpp"
type: test
path: tests/vendor/test_vendor_centred_cluster.cpp
domain: tests
bucket: vendor
loc: 81
classes: []
sources: ["tests/vendor/test_vendor_centred_cluster.cpp"]
---
# `test_vendor_centred_cluster.cpp`

> **一句定位**：驗證幕間市集十個攤位的版面（兩排各五、廣場圓盤內、互不重疊）以及每攤對應唯一精靈（無十個分身）。

## 職責

本測試從真實內容檔（`TEST_CONTENT_DIR`）載入市集攤位資料，驗證幾何版面與精靈唯一性。

**版面測試**（`市集攤位橫跨廣場排成兩排各五攤`）：
- `ChapterVendors(Interlude_Market).size() == 10`。
- 恰好兩個不同的 Y 值（兩排），每排各 5 攤；兩排 Y 差距 ≥ 60px（走道寬度）。
- 所有攤位距羅馬廣場圓心（約 1088, 960）≤ 160px（廣場圓盤內）。
- 任兩攤之間距離 > 30px（互不重疊，碰撞箱 24px 的舒適下限）。

**精靈唯一性測試**（`每個市集攤位都對應到相異的精靈`）：
- 10 個攤位的精靈 key（`VendorSpriteKey(stallKeeper, name)`）兩兩相異。
- 10 個攤位實際指派的精靈（`VendorSpriteFor(i, stallKeeper, name, pos)`）也兩兩相異，與正式產品 `World::SpawnChapterNpcs` 使用的選擇器完全相同。

需要建置系統定義 `TEST_CONTENT_DIR`。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::ChapterVendors(SemesterState)` — 被測：回傳攤位清單（含 pos + config）。
- `nccu::SetVendorContentDir(dir)` / `nccu::ReloadVendors()` — fixture 設定函式。
- `nccu::VendorSpriteKey(stallKeeper, name)` — 從 VendorSprite.h，用於唯一性驗證。
- `nccu::VendorSpriteFor(idx, stallKeeper, name, pos)` — 實際精靈選擇器，與產品代碼完全相同。
- `std::map<float, int> rows` — 用於計數每個 Y 值的攤位數量（兩排各五）。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/ChapterVendors.h`、`game/vendor/VendorSprite.h`、`game/state/SemesterState.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：World 生成層測試，驗證市集版面的幾何不變量。

## OO 概念與設計重點

測試直接使用與產品代碼相同的 `VendorSpriteFor`（而非另外建立替身），確保測試與執行時行為一致。幾何不變量測試（圓盤內、不重疊）是對「內容作者可能移動攤位」情況的守門測試——Tiled 重新排版後此測試必須立即失敗。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/vendor/test_vendor_centred_cluster.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/vendor/test_vendor_centred_cluster.cpp) · [← 全檔索引](../files-index.md)
