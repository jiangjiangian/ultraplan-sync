---
id: "file:include/engine/render/Texture.h"
type: header
path: include/engine/render/Texture.h
domain: engine
bucket: render
loc: 190
classes: [Texture, TextureCache]
sources: ["include/engine/render/Texture.h"]
---
# `Texture.h`

> **一句定位**：raylib GPU 材質的 move-only RAII 控制代碼，搭配程序級材質快取（`TextureCache`）——每個路徑至多上傳一次 GPU、返回非擁有「檢視」，確保 double-free / use-after-free 不可能發生。

## 職責

`Texture.h` 是整個引擎材質管理的核心，在 engine render 層以一份 190 行的標頭解決了三個相互關聯的問題：記憶體安全（RAII）、GPU 資源唯一性（每路徑一份）、缺檔不崩潰（sentinel no-op）。

**所有權模型**：`Texture` 以 `owns_` 旗標區分「擁有者」（解構時 `::UnloadTexture`）與「非擁有檢視」（解構為 no-op）。快取（`detail::TextureCache`）是唯一的擁有者；所有遊戲實體（`Player`、`NPC`）和 View 元件持有的 `Texture` 都是非擁有檢視，其解構絕不觸碰共享 GPU id。

**材質快取（`detail::TextureCache`）**：以路徑為鍵的 `unordered_map`，以 function-local static 方式惰性建立（`Textures()` 函式）。首次 `Acquire(path)` 上傳並建立擁有者，後續同路徑的 `Acquire` 直接回傳非擁有檢視，不再讀磁碟——即使重建 World / View 也命中暖快取，無卡頓。`ShutdownTextureCache()` 在 `main.cpp` 於 `CloseWindow` 之前明確呼叫，確保 GL context 仍存活時 Unload 所有材質，與 `ShutdownFont` 採相同紀律。

**缺檔契約**：缺檔時 raylib 回傳 `Texture2D{id=0}`；快取存入一筆無效擁有者（`owns_=false`，以免 Unload id=0）並回傳無效檢視（`IsValid()==false`）。呼叫 `Renderer::Texture`/`TextureRect` 時 `id=0` 的材質自然不畫任何東西，整條路徑為 no-op，不崩潰——支援無頭自動遊玩與空資源路徑。

**三個對外自由函式**：`PreloadTexture(path)`（暖快取、可重複呼叫）、`ShutdownTextureCache()`（明確拆除、冪等）、`TextureCacheSize()`（診斷用）。`Texture::Load(path)` 靜態方法委派給 `detail::Textures().Acquire(path)`。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class Texture` | Move-only RAII；`owns_` 旗標；禁止拷貝；`~Texture` 條件式 Unload。 |
| `Texture::Load(const std::string& path)` | 靜態工廠；委派快取，回傳非擁有檢視。 |
| `Texture::Width()` / `Height()` / `IsValid()` | 查詢材質屬性；`noexcept`。 |
| `Texture::Raw()` | 供 `Renderer` 存取底層 `::Texture2D`；不對外暴露 GPU id 概念。 |
| `class detail::TextureCache` | 路徑 → 擁有者 map；`Acquire(path)`（找或建）、`Warm(path)`（預載）、`Clear()`（拆除）、`Size()`（診斷）。 |
| `detail::Textures()` | Function-local static 快取的全局存取點。 |
| `PreloadTexture(path)` | inline 自由函式；呼叫 `Textures().Warm(path)`。 |
| `ShutdownTextureCache()` | inline 自由函式；呼叫 `Textures().Clear()`；冪等。 |
| `TextureCacheSize()` | inline 自由函式；回傳快取筆數，供測試斷言。 |

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（直接使用 `::LoadTexture`/`::UnloadTexture`/`::Texture2D`）、標準庫 `<string>`、`<unordered_map>`。
- **被誰使用（往內）**：`include/engine/render/RaylibRenderer.h`（`DrawSprite` 傳入 `const Texture&`）、`include/engine/render/Renderer.h`（`Texture`/`TextureRect` 方法參數）、`include/game/entities/NPC.h`/`Player.h`（sprite 成員）、`include/game/world/TexturePreload.h`（啟動期暖快取）、`include/app/scenes/CharacterSelectScene.h`、`include/ui/View.h`、`src/app/main.cpp`、`tests/gfx/test_texture_cache.cpp`。
- **繼承 / 實作 / 體現**：體現 [RAII / 記憶體安全（oo-raii）](../concepts/oo-raii.md)。
- **每幀管線 / MVC 角色**：Engine 基礎設施；不直接參與每幀管線，但所有在每幀繪製的 sprite 都透過 `Texture` 攜帶 GPU 控制代碼。

## OO 概念與設計重點

`Texture` 是書教科書級的 [RAII（oo-raii）](../concepts/oo-raii.md) 範例：資源（GPU 材質 id）的擁有權在物件生命週期內管理，解構時條件式釋放，確保資源絕不洩漏也絕不雙重釋放。Move-only（禁止拷貝、允許 move）確保所有權轉移語意清晰。

「擁有者 vs 非擁有檢視」的雙重角色設計是本設計最精妙之處：它讓快取（程序範圍的 function-local static）當唯一擁有者，其餘所有持有者都是「廉價的別名」，兼顧了安全性（無 double-free）與效能（無重複上傳）。此模式類似 `std::shared_ptr` 的語意，但成本更低（無 atomic 引用計數）、邊界更明確（快取之外永遠不擁有）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/Texture.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Texture.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
