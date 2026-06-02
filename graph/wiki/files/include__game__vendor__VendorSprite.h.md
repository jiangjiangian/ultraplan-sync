---
id: file:include/game/vendor/VendorSprite.h
type: header
path: include/game/vendor/VendorSprite.h
domain: game
bucket: vendor
loc: 79
classes: []
sources: ["include/game/vendor/VendorSprite.h"]
---
# `VendorSprite.h`

> **一句定位**：市集攤販 sprite 選圖的純規則層——以生成索引而非雜湊鍵保證十攤各取不同外觀，不需 GL 環境即可單元測試。

## 職責

本檔解決了舊版「十個攤位全用同一張 shop_auntie.png」的缺陷，同時避免雜湊碰撞（生日悖論）在有 PIPOYA 美術包時也衝突。核心思路是改以**生成索引**（`index`）做選圖的唯一依據，讓每一攤位對應到名冊裡的不同位置，而非用 `(key, pos)` 雜湊後碰撞。

兩條選圖路徑都走相同邏輯：有 PIPOYA 包時索引進 `PipoyaRoster()`；無包時索引進本檔定義的 `kVendorFallbackSprites`（10 張精選後備，涵蓋 shop_auntie、suit_senior、ta 與多套制服），兩份清單都有 ≥10 項足供十攤各取不同。

`VendorSpriteKey()` 仍保留以攤主或攤名組出 `"vendor:<name>"` 形式的鍵，維持舊接口相容性（給仍走 `PickNpcSprite` 的路徑使用），但 `VendorSpriteFor()` 本身已完全忽略此鍵改走索引。

整個標頭都是 `inline` 函式與 `inline constexpr` 陣列，不需 GL，因此可直接在 `tests/vendor/test_vendor_centred_cluster.cpp` 中做純邏輯的單元測試。

## 關鍵內容（類別 / 函式 / 資料）

- `kVendorFallbackSprites[]`（`inline const char* const`，10 項）：無 PIPOYA 包時的後備 sprite 路徑陣列，涵蓋 shop_auntie、suit_senior、ta 與多張制服圖（female_04/07/10/13、male_04/07/10）。
- `kVendorFallbackCount`（`inline constexpr std::size_t`）：後備陣列長度（== 10），供索引取模用。
- `VendorSpriteKey(stallKeeper, name) -> std::string`：組出 `"vendor:<攤主或攤位名>"` 鍵——攤主非空用攤主，否則用攤位名；保證各攤唯一，供 PickNpcSprite 舊路徑調用。
- `VendorSpriteFor(index, stallKeeper, name, pos) -> std::string`：實際選圖邏輯——調用 `PipoyaRoster()`，非空則取 `roster[index % roster.size()]`，空則取 `kVendorFallbackSprites[index % kVendorFallbackCount]`。`stallKeeper`、`name`、`pos` 三個參數以 `[[maybe_unused]]` 標記，保留簽名以待未來需要但當前不使用。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/PipoyaRoster.h`（取得 PIPOYA 多樣 NPC 名冊，若空用後備陣列）；`engine/math/Vec2.h`（`pos` 參數型別，當前未實際使用）。
- **被誰使用（往內）**：`include/game/world/TexturePreload.h`（預熱後備 sprite 快取）；`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（生成攤販 NPC 時呼叫）；`tests/vendor/test_vendor_centred_cluster.cpp`（單元測試驗證十攤各異）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 game 領域 / vendor bucket，是 Spawn 階段（`SpawnChapterNpcs`）的純資料輔助函式，不參與任何幀管線系統。

## OO 概念與設計重點

本檔是典型的 **header-only 純函式工具集**：所有邏輯都是 `inline` 自由函式，無狀態、無 class，可直接在無 GL 環境中測試。設計上刻意將「選圖規則」從 `WorldSpawn.cpp` 抽離成獨立可測單元，體現 **SRP（單一職責）**——攤販外觀邏輯與世界生成邏輯分居不同編譯單元。以生成索引取代雜湊鍵的設計是一種**確定性保證**：任意環境（有無美術包）下相同索引總是得到相同外觀，使自動跑流程的存檔在視覺上穩定可重現。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/vendor/VendorSprite.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorSprite.h) · [← 全檔索引](../files-index.md)
