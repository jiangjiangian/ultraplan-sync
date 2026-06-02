---
id: file:include/game/gfx/MaskLoader.h
type: header
path: include/game/gfx/MaskLoader.h
domain: game
bucket: gfx
loc: 62
classes: []
sources: ["include/game/gfx/MaskLoader.h"]
---
# `MaskLoader.h`

> **一句定位**：把地形可行走 PNG 載入為不依賴 raylib 的 `CollisionMask`，支援 primary/fallback 雙路徑與白底/透明底兩種匯出格式。

## 職責

`MaskLoader.h` 提供一個 `inline` 函式 `LoadCollisionMask(primary, fallback)`，把手繪地形遮罩 PNG 解碼為 `CollisionMask`（位元陣列），供 `TerrainMask.cpp` 在遊戲初始化時載入。

**雙路徑容錯**：先嘗試 `primary`（手繪精確遮罩），缺檔時退回 `fallback`（工具自動生成、僅含建築+河流的簡略版）。兩者皆缺時輸出 `[CollisionMask] WARNING` 至 `stderr` 並回傳空遮罩（全部可行走），讓地形碰撞「明顯」降級而非無聲失效。

**雙格式支援**：可行走判定：`alpha == 0`（完全透明）或 `(r,g,b) == (255,255,255)`（純白）才可行走，其餘視為實心。此雙重規則容納「RGB 白底匯出」（美術在 Aseprite 壓平圖層）與「RGBA 透明底匯出」（PNG 預設透明底）兩種來源，讓美術不論如何匯出都不會把整個世界悄悄變成實心。

**無 raylib 相依**：raylib::Image 的生命週期與解碼封在 `engine/render/ImageDecoder.cpp`；本標頭僅使用 `nccu::engine::render::DecodedImage` 中間格式（純 `vector<uint8_t> rgba8`），不引入任何 raylib 符號，確保遮罩載入邏輯可在無 GL context 的環境中測試。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline CollisionMask LoadCollisionMask(const std::string& primary, const std::string& fallback)`**（`namespace nccu::game::gfx`）：
  - 呼叫 `LoadRgba8Image(primary)`；缺檔時退回 `fallback`；兩者皆缺時警告 + 空遮罩。
  - 可行走判定：`a==0 || (r==255 && g==255 && b==255)` → `solid[i] = 0`，否則 `= 1`。
  - 成功時向 `stderr` 輸出 `"[CollisionMask] loaded WxH from 'path'"` 的診斷訊息。
  - 回傳 `CollisionMask{w, h, std::move(solid)}`。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/render/ImageDecoder.h`（`LoadRgba8Image`、`DecodedImage`）、`include/game/world/CollisionMask.h`（回傳型別）；`<cstdio>`（`fprintf`）、`<string>`、`<vector>`。
- **被誰使用（往內）**：`src/game/world/TerrainMask.cpp`（唯一呼叫端）。
- **繼承 / 實作 / 體現**：—（純函式，無類別）。
- **每幀管線 / MVC 角色**：初始化時呼叫一次（非每幀）。`TerrainMask.cpp` 在遊戲啟動時載入遮罩，存入 `World`；`NPC::SetWanderMask` 傳入此遮罩讓漫步 NPC 避牆；Collision System 以遮罩阻擋玩家穿牆。

## OO 概念與設計重點

`LoadCollisionMask` 是跨架構層（`engine/render` 的解碼 ↔ `game/world` 的碰撞遮罩）的橋接函式，放在 `game/gfx` 層是合理的中介位置。`inline` 實作封裝 raylib 的 Image API 於 `ImageDecoder` 後，維持「game 層不直接引入 raylib」的架構紅線（[DIP 渲染器](../concepts/arch-dip-renderer.md)）。

雙格式可行走判定（透明 OR 白）是防禦性設計：允許美術在不知道技術規格的情況下以任意常見格式匯出，避免「整個世界變成實心」的靜默失效。`primary/fallback` 雙路徑讓新 checkout（缺手繪精確遮罩）仍能使用自動生成版繼續開發。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/MaskLoader.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/MaskLoader.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP 渲染器](../concepts/arch-dip-renderer.md)
