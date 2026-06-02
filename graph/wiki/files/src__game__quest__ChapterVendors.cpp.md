---
id: file:src/game/quest/ChapterVendors.cpp
type: source
path: src/game/quest/ChapterVendors.cpp
domain: game
bucket: quest
loc: 142
classes: []
sources: ["src/game/quest/ChapterVendors.cpp"]
---
# `ChapterVendors.cpp`

> **一句定位**：各章節攤販的空間擺放來源——市集插曲解析 Markdown 後配上硬編碼座標，Ch2 圖書館自販機與 Ch4 集英樓便利商店以手寫固定攤位定義。

## 職責

本檔在 `nccu` 命名空間內維護三個章節的攤販擺放資料，以 `VendorPlacement`（`VendorConfig` + `Vec2 position`）的向量回傳，由 `World::SpawnChapterNpcs` 消費。

**市集插曲（`BuildInterlude` / `Chapter2Vendors`）**：惰性載入函式，以 `std::optional<vector<VendorPlacement>>` 快取解析結果。首次呼叫時以 `nccu::vendor::LoadInterludeVendors(VendorContentDir() + "/interlude_market.md")` 解析內容檔，再與 10 個固定座標（`InterludeStallPositions`：羅馬廣場兩排各五個，間距 54px，經遮罩驗證可到達）配對。座標超出 configs 數量的閒置，configs 超出座標數量的一律用最後一個位置。`SetVendorContentDir` / `ReloadVendors` 可重置快取。

**座標說明**：北排 y=900、南排 y=1020，中心偏西 36px 以避開廣場東北/東南角的牆（可行走範圍東側到 x≈1160）。每次建置由可達性測試重新檢查攤位座標。

**Ch2 圖書館地下室自動販賣機（`Chapter2Vendors`）**：手寫單一 `VendorPlacement`，`VendorConfig` 含名稱「圖書館地下室自動販賣機」、`EnergyDrink` 售 35 元（庫存無限，`-1`）、`spriteOverride` 指向 Machine 1 美術、`npcId = kNpcCh2Vendor`（`!` 指引共用鍵）。位置 `{980, 560}`，置於中正圖書館正面，與命名相符。

**Ch4 集英樓便利商店（`Chapter4Vendors`）**：手寫單一 `VendorPlacement`，販售 `"UglyUmbrella"` 售 100 元（`setsFlag = kFlagBoughtUglyUmbrella`，Ending C 觸發點）、spriteOverride 指向 Machine 3、位置 `{1500, 1450}`，置於集英樓矩形西側面。

**`ChapterVendors(state)`**：根據 `SemesterState` 路由到對應的攤販向量（其他章節回傳空向量；Ch3 沒有附帶 Vendor，任務是 NPC 物物交換）。

## 關鍵內容（類別 / 函式 / 資料）

- `InterludeStallPositions()` → `const vector<Vec2>&`：函式內 static，10 個廣場座標。
- `InterludeCache()` → `optional<vector<VendorPlacement>>&`：惰性快取，可 reset 強制重新解析。
- `BuildInterlude()` → `const vector<VendorPlacement>&`：解析內容檔 + 配對座標。
- `Chapter2Vendors()` → `const vector<VendorPlacement>&`：手寫 Ch2 自販機，`npcId=kNpcCh2Vendor`。
- `Chapter4Vendors()` → `const vector<VendorPlacement>&`：手寫 Ch4 便利商店，`setsFlag=kFlagBoughtUglyUmbrella`。
- `ChapterVendors(SemesterState state)` → `const vector<VendorPlacement>&`：統一路由入口。
- `SetVendorContentDir(string)` / `ReloadVendors()`：測試/熱重載接縫。
- `VendorContentDir()`：函式內 static，預設 `"docs/content"`。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterVendors.h`、`Chapter2Quest.h`（`kNpcCh2Vendor`）、`Flags.h`（`kFlagBoughtUglyUmbrella`）、`VendorLoader.h`（`LoadInterludeVendors`）、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點；由 `World::SpawnChapterNpcs` 呼叫，在章節進場時生成攤販實體）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：章節進場初始化階段（非每幀管線），但 `kFlagBoughtUglyUmbrella` 的 `VendorItem::setsFlag` 在每次 Vendor 購買確認時由 `Vendor::TryBuy` 套用，進而影響 `CheckEndingGates`。

## OO 概念與設計重點

本檔的核心設計哲學是「空間擺放是 code 的職責，內容（文案/商品）是內容檔的職責」，與章節對白的分工相同。`optional<vector<VendorPlacement>>` 的惰性快取使首次呼叫才觸發磁碟讀取，且 `reset()` 允許測試在不重啟進程的前提下重新解析。Ch4 集英樓便利商店的 `setsFlag = kFlagBoughtUglyUmbrella` 是 Ending C 的資料驅動觸發點，體現了「結局條件嵌入道具定義而非硬編碼在互動邏輯中」的設計。[Factory Method](../concepts/pat-factory.md) 的輕量形式：`ChapterVendors(state)` 根據狀態回傳不同的攤販集合。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/ChapterVendors.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/ChapterVendors.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md) · [State](../concepts/pat-state.md)
