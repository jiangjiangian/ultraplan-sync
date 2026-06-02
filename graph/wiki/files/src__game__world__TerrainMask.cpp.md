---
id: file:src/game/world/TerrainMask.cpp
type: source
path: src/game/world/TerrainMask.cpp
domain: game
bucket: world
loc: 19
classes: []
sources: ["src/game/world/TerrainMask.cpp"]
---
# `TerrainMask.cpp`

> **一句定位**：`LoadTerrainMask` 的實作——跨越 gfx 層的唯一橋接點，讓 `World`（Model 層）能取得像素精確的碰撞遮罩卻無需直接相依 raylib。

## 職責

此檔屬於 game / world 層，只有一個函式 `LoadTerrainMask()`，它呼叫 `nccu::game::gfx::LoadCollisionMask(...)` 讀取兩個 PNG 遮罩檔（`collision_mask.png` 疊加在 `collision_mask_base.png` 之上，基底含建築牆基與河流，上層含樹木、花圃、外牆），回傳一個 `CollisionMask` 物件。

此檔是整個專案中「唯一允許」在非 gfx 編譯單元透過 gfx 載入器取用 raylib（進行像素讀取）的地方。它的存在使 `World.cpp` 和其他模型層編譯單元完全不需要 include 任何 gfx 或 raylib 標頭，同時又能讓 `World` 在構建時呼叫 `LoadTerrainMask()` 取得遮罩，保持 `World` 對 raylib 的零相依。

## 關鍵內容（類別 / 函式 / 資料）

- `LoadTerrainMask()` — 公開自由函式；固定讀取 `"resources/assets/maps/collision_mask.png"` 和 `"resources/assets/maps/collision_mask_base.png"` 兩個 PNG，回傳 `CollisionMask`。

## 相依與在架構中的位置

- **#include（往外）**：`CollisionMask.h`（回傳型別）、`MaskLoader.h`（gfx 層的 `LoadCollisionMask`，後者相依 raylib）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `World.cpp` 建構式中 `terrainMask_ = LoadTerrainMask()` 呼叫一次。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（初始化時執行，非每幀路徑）；屬 Model / game 層的載入輔助，使碰撞遮罩資料與 raylib 載入機制解耦。

## OO 概念與設計重點

此檔體現了 [DIP（相依反向）](../concepts/arch-dip-renderer.md) 的精神：Model 層（`World`）透過一個薄的自由函式介面取得遮罩資料，而不直接 include gfx / raylib 相關標頭，維持「engine 不反向相依」的架構紅線。整個檔案僅 19 行，是最小化「層間橋接」的典型應用，也說明了「哪怕只有一行有用的呼叫，也值得用獨立編譯單元隔離層」。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/TerrainMask.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/TerrainMask.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
