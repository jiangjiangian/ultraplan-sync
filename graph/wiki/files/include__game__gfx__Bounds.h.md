---
id: file:include/game/gfx/Bounds.h
type: header
path: include/game/gfx/Bounds.h
domain: game
bucket: gfx
loc: 38
classes: []
sources: ["include/game/gfx/Bounds.h"]
---
# `Bounds.h`

> **一句定位**：把實體鉗制在世界邊界內的純幾何工具函式，無 raylib、無模擬狀態，header-only。

## 職責

`Bounds.h` 只提供一個 `inline` 函式 `ClampToWorld(pos, size, worldSize)`，將 AABB 錨點（左上角座標）鉗制在 `[0, worldSize - size]` 範圍內，確保物件的碰撞盒完整落在世界邊界內。

關鍵邊界處理：某軸上 `size > worldSize` 時（物件比世界還寬），無有效夾制區間，該軸釘在 0（取下界），避免回傳負座標。實作以 `lambda clamp1` 封裝單軸夾制邏輯，再對 `x`、`y` 各呼叫一次。

純幾何函式：不依賴 raylib、不持有模擬狀態、不讀輸入、不發事件。`noexcept` 標記確保無例外傳播。

用途：`GameController::GameController` 以 `ClampToWorld` 確保玩家和 NPC 移動後不超出地圖邊界；`SimSystems` 的 Collision System 在碰撞解析後呼叫以修正位置。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline Vec2 ClampToWorld(Vec2 pos, Vec2 size, Vec2 worldSize) noexcept`**（`namespace nccu::game::gfx`）：將 AABB 錨點鉗制在世界邊界內；`size > worldSize` 軸釘 0。
- 內部 `lambda clamp1(v, lo, hi)`：單軸夾制，`hi < lo` 時返回 `lo`（防退化）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`（輸出入型別）。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（移動後邊界修正）、`src/game/controller/SimSystems.cpp`（碰撞解析後修正）、`tests/gfx/test_bounds.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—（無類別，純函式）。
- **每幀管線 / MVC 角色**：Movement System 與 Collision System 管線的末端修正步驟。純幾何計算，在 Model 層外沒有任何副作用。

## OO 概念與設計重點

`Bounds.h` 是 header-only 純幾何工具的典型形式：`inline` 函式使編譯器可以內聯展開、無連結器符號；`noexcept` 承諾無例外開銷；`namespace nccu::game::gfx` 防止命名空間汙染。不依賴 raylib 意味著此函式可在無 GL context 的無頭測試中直接呼叫（`tests/gfx/test_bounds.cpp`），是「純函式可測試性」的設計體現。

`hi < lo` 的防退化處理（物件比世界大時釘 0）是關鍵邊界條件：若缺少此處理，回傳負座標會讓 NPC 或玩家瞬間跳到地圖外。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/Bounds.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/Bounds.h) · [← 全檔索引](../files-index.md)
