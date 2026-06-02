---
id: file:include/game/world/TexturePreload.h
type: header
path: include/game/world/TexturePreload.h
domain: game
bucket: world
loc: 117
classes: []
sources: ["include/game/world/TexturePreload.h"]
---
# `TexturePreload.h`

> **一句定位**：進入遊戲前以 World／View 即將請求的全部貼圖預熱快取，消除開場（與重新開始）時讀檔與 GPU 上傳造成的單幀卡頓。

## 職責

本標頭定義了 `game::world::PreloadGameTextures()`，在 `LoadingScene` 建構時呼叫一次，把 World 建構（`LoadSprite`）與 View 建構（底圖、建築、裝飾條）即將請求的所有貼圖預先上傳至 GPU，使後續的實際構建與渲染命中已預熱的快取而非即時讀盤。重新開始時建立全新的 World／View，但快取已預熱，不再卡頓。

預熱分七個批次，且刻意使用「與 View.cpp、World.cpp、CharacterSelect.cpp 傳給 `Texture::Load` 完全相同的路徑與相同的正規表」，避免清單默默走樣：
1. 世界底圖 `worldmap_base.png`（最大單一貼圖）。
2. 建築美術——遍歷 `buildings::kAll`，跳過 `obstacles::kBuildingCollisionSkip` 名單，以 `BuildingTexturePath(name)` 組出路徑。
3. 環境裝飾條——遍歷 `nccu::game::gfx::kDecorations`，取 `stripPath`。
4. 五張角色圖（`kPersonas`）。
5. 攤販後備名冊（`kVendorFallbackSprites`，10 張）。
6. 精選劇情 NPC 圖（`kCuratedSprites`，16 張）。
7. PIPOYA 多樣 NPC 名冊（`PipoyaRoster()`，有包時整批預熱；全新 clone 為空迴圈）。

操作為冪等且在全新 clone 上幾乎零成本（快取未命中為無操作），故此函式可於啟動時安全呼叫；必須在 `InitWindow` 之後呼叫（需 GPU 存取），呼叫端以視窗存在與否守衛。

## 關鍵內容（類別 / 函式 / 資料）

- `kWorldmapBasePath`（`inline constexpr std::string_view`）：世界底圖路徑 `"resources/assets/maps/worldmap_base.png"`，與 View 傳給 `Texture::Load` 的路徑完全一致。
- `kCuratedSprites`（`inline constexpr std::array<std::string_view, 16>`）：精選劇情 NPC 與制服圖路徑（shop_auntie、suit_senior、ta 與 school_uniform_3 系列），補足全新 clone 上不存在 PIPOYA 包時的預熱集合。
- `BuildingTexturePath(name) -> std::string`（`inline`）：組出 `"resources/assets/buildings_3d_trimmed/<name>.png"`，與 View 的組法一致。
- `PreloadGameTextures()`（`inline`）：七批貼圖預熱的主函式，遍歷 `kAll`、`kDecorations`、`kPersonas`、`kVendorFallbackSprites`、`kCuratedSprites`、`PipoyaRoster()` 依序呼叫 `nccu::engine::render::PreloadTexture`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Texture.h`（`PreloadTexture` 函式）；`game/gfx/Decorations.h`（裝飾條定義 `kDecorations`）；`game/entities/Personas.h`（角色圖 `kPersonas`）；`game/world/Buildings.h`（建築表 `kAll`）；`game/world/Obstacles.h`（略過名單）；`game/vendor/VendorSprite.h`（後備攤販 sprite）；`game/quest/PipoyaRoster.h`（多樣 NPC 名冊）；`<algorithm>`（`std::find`）。
- **被誰使用（往內）**：`src/app/scenes/LoadingScene.cpp`（唯一呼叫端，進入遊戲前觸發）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：不在每幀管線中，而是在 `LoadingScene` 一次性執行；屬於 app 層（組裝根）呼叫的 game 層輔助函式，維持 MVC 乾淨——它只讀取靜態美術表，不碰任何遊戲狀態。

## OO 概念與設計重點

`PreloadGameTextures()` 的設計核心是 **單一真實來源**：它直接引入 View.cpp 與 World.cpp 使用的同一份靜態表（`kAll`、`kPersonas`、`kDecorations` 等），確保預熱的路徑集合與實際載入的路徑集合不漂移。作為 header-only 的 `inline` 函式，它可被不同的場景直接呼叫而無需連結新的 `.cpp` 單元，符合輕量工具函式的設計慣例。冪等設計（多次呼叫結果相同）使其可安全地在重新開始流程中再次被呼叫，體現了 **健壯性設計** 原則。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/TexturePreload.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/TexturePreload.h) · [← 全檔索引](../files-index.md)
