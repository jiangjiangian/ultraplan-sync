---
id: file:tests/gfx/test_texture_cache.cpp
type: test
path: tests/gfx/test_texture_cache.cpp
domain: tests
bucket: gfx
loc: 79
classes: []
sources: ["tests/gfx/test_texture_cache.cpp"]
---
# `test_texture_cache.cpp`

> **一句定位**：在無 GL context 的環境下驗證貼圖快取的「載入一次即快取」語意——重複載入命中快取、缺檔為乾淨 no-op（id==0、不崩潰）、不同路徑各新增一筆，以及 `PreloadTexture` 預熱機制。

## 職責

此測試利用 ctest 無 GL context 的特性（raylib 的 `::LoadTexture` 對所有路徑回傳 `Texture2D{0}`）來驗證快取層的「同一性」契約，而非驗證真正的貼圖像素。四個 TEST_CASE 各自斷言一個具體的快取行為，使用合成路徑前綴（`test::__`）避免與其他測試的快取狀態衝突。

特別值得注意的是「缺檔 no-op」不應崩潰的合約：快取持有一個 id==0 的擁有者，`Texture::Load` 回傳非擁有視圖，兩個 handle 離開作用域時均不呼叫 `::UnloadTexture`，防止雙重釋放。

## 關鍵內容（類別 / 函式 / 資料）

- `ng::TextureCacheSize()`：回傳當前快取條目數，用於斷言命中 vs 未命中。
- `ng::Texture::Load(path)`：主要 API，回傳快取中該路徑的非擁有視圖。
- `ng::PreloadTexture(path)`：預熱快取但不回傳 handle。
- `TEST_CASE("Texture 快取：對同一路徑重複 Load 為命中（不重新載入）")`：前後 `TextureCacheSize` 差 1，兩次回傳的 `Raw().id` 相同。
- `TEST_CASE("Texture 快取：缺檔為乾淨的 no-op（契約不變）")`：`IsValid()==false`、`Raw().id==0`；取第二個 handle 不崩潰。
- `TEST_CASE("Texture 快取：不同路徑各新增恰好一筆")`：兩個不同路徑後快取大小增 2；再次 Load 兩個路徑後大小不變。
- `TEST_CASE("Texture 快取：PreloadTexture 預熱快取但不回傳 handle")`：預熱後 size++；之後 Load 相同路徑 size 不變；仍 `IsValid()==false`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/Texture.h`（唯一依賴，提供 `Texture`、`TextureCacheSize`、`PreloadTexture`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（engine 資源管理測試）

## OO 概念與設計重點

快取以全域 `static` map（或等價結構）實作，測試透過 `TextureCacheSize()` 間接觀察其狀態。使用合成路徑前綴 `test::__` 是避免測試間快取汙染的重要慣例，體現了測試隔離原則。「缺檔不崩潰」的明確驗證防禦了 raylib `UnloadTexture(0)` 的潛在 UB，是資源管理的關鍵安全保障。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_texture_cache.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_texture_cache.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
