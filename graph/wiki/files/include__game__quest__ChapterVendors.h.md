---
id: file:include/game/quest/ChapterVendors.h
type: header
path: include/game/quest/ChapterVendors.h
domain: game
bucket: quest
loc: 54
classes: [VendorPlacement]
sources: ["include/game/quest/ChapterVendors.h"]
---
# `ChapterVendors.h`

> **一句定位**：以 `SemesterState` 為鍵回傳各章節攤販配置（`VendorPlacement` 向量），是 `ChapterNpcSpawns` 的「價目表姊妹」，為 World 的 `RespawnChapterRoster` 提供攤位資料。

## 職責

`ChapterVendors.h` 宣告 `VendorPlacement` 結構體（`VendorConfig` + 世界座標 `pos`）以及三個函式：`ChapterVendors(SemesterState)`、`SetVendorContentDir(string)` 和 `ReloadVendors()`，處理攤販配置的執行期解析與快取。

Vendor 物件與普通 NPC 不同，它需要 `VendorConfig`（含商品列表與台詞）而非單純的 sprite 路徑 + npcId，故另立此表。幕間（`Interlude_Market`）的攤販不再是程式端手寫字面值，而是在執行期由 `LoadInterludeVendors` 解析 `interlude_market.md` 後快取，讓市集內容（攤名、商品、台詞）以 Markdown 為單一真實來源，改 .md 即可修改，無須重新編譯。

`SetVendorContentDir` 供測試指向測試用內容目錄（與 `dialog::SetContentDir` 平行的設計），呼叫後使快取失效。`ReloadVendors` 是熱重載入口，作為 `dialog::Reload()` 的姊妹。其他非幕間狀態目前回傳空向量，預留給日後各章附帶攤販。

## 關鍵內容（類別 / 函式 / 資料）

- `struct VendorPlacement`：攤販位置 POD，含 `VendorConfig config` 與 `Vec2 pos` 世界座標。
- `ChapterVendors(SemesterState state) → const std::vector<VendorPlacement>&`：分派函式，幕間由解析器供應（首次呼叫時解析並快取），其餘狀態回傳空靜態向量。
- `SetVendorContentDir(std::string dir)`：設定 markdown 內容目錄，使快取失效（預設 `"docs/content"`）。
- `ReloadVendors()`：丟棄解析快取，下次呼叫 `ChapterVendors()` 重新讀取 .md（熱重載）。

## 相依與在架構中的位置

- **#include（往外）**：`VendorConfig.h`（`VendorConfig`/`VendorItem` 結構體）、`SemesterState.h`（分派鍵）、`Vec2.h`（世界座標）
- **被誰使用（往內）**：`src/game/quest/ChapterVendors.cpp`（實作體）、`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（`RespawnChapterRoster`）；攤販相關測試（`test_vendor_centred_cluster.cpp`、`test_vendor_decline.cpp`、`test_vendor_loader.cpp`、`test_ch1_spine_reachable.cpp`、`test_spawn_reachability.cpp`）以及 `test_i35_interact_vendor.cpp`
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層資料供應——由 World Spawn 管線在章節轉移時讀取以生成 Vendor 物件；`ReloadVendors` 為熱重載支援（遊戲開發期使用）。

## OO 概念與設計重點

「內容以檔案為單一真實來源」的設計原則：幕間市集攤販資料存於 Markdown 檔案，而非硬編碼在 C++ 中，使企劃能在不觸碰程式碼的情況下調整市集內容。這與章節對話 `LoadChapter` 的「降級為空」契約一致。`SetVendorContentDir` + `ReloadVendors` 的測試注入點是依賴注入（DIP）在純函式層面的輕量體現，無需修改函式簽章就能替換內容目錄。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ChapterVendors.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterVendors.h) · [← 全檔索引](../files-index.md)
