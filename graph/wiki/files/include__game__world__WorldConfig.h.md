---
id: file:include/game/world/WorldConfig.h
type: header
path: include/game/world/WorldConfig.h
domain: game
bucket: world
loc: 23
classes: []
sources: ["include/game/world/WorldConfig.h"]
---
# `WorldConfig.h`

> **一句定位**：世界地圖邊長與玩家碰撞盒尺寸的編譯期常數，供 Controller、NPC 與 View 共用以避免各自硬編不一致的數值。

## 職責

本標頭定義三個 `inline constexpr float`，集中管理兩類關鍵的世界尺寸參數：世界地圖邊長 `kSize = 2048.0f` 與玩家碰撞盒 `kPlayerWidth = kPlayerHeight = 24.0f`。

這些數值在多個編譯單元中都需要使用：`GameController.cpp` 需要知道玩家碰撞盒大小以計算移動解算的 AABB；`NPC.cpp` 在移動時也需要此值；`View.cpp` 需要世界尺寸來設定相機範圍；`QuestGiverIndicators.cpp` 在計算顯示範圍時也依賴它。若各自硬編，任一處調整都必須同步修改所有消費端，且沉默漂移時難以偵測。

注解明確指出：若 `worldmap_base.png` 被重新調整尺寸，`kSize` 須同步更新；`kPlayerWidth/kPlayerHeight` 也須與 `Player.cpp` 的 24×24 hit-box 一致——這些是「必須人工同步的跨檔案約束」，集中定義讓維護者在一個地方就看到所有需要同步的值。

值得注意的是，本標頭使用的命名空間是 `world`（非 `nccu` 或 `nccu::world`），在項目中是較不尋常的選擇。

## 關鍵內容（類別 / 函式 / 資料）

- `world::kSize`（`inline constexpr float = 2048.0f`）：世界地圖邊長（正方形），對應 `worldmap_base.png` 的實際像素尺寸。
- `world::kPlayerWidth`（`inline constexpr float = 24.0f`）：玩家碰撞盒寬度，須與 `Player.cpp` 的 hit-box 一致。
- `world::kPlayerHeight`（`inline constexpr float = 24.0f`）：玩家碰撞盒高度，須與 `Player.cpp` 的 hit-box 一致。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 include）。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（玩家移動解算 AABB 尺寸）；`src/game/entities/NPC.cpp`（NPC 移動解算）；`src/ui/View.cpp`（相機範圍設定）；`src/ui/world/QuestGiverIndicators.cpp`（顯示範圍計算）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純常數，無管線行為。被 Controller、NPC（Movement 階段）與 View（渲染）各自引用。

## OO 概念與設計重點

本檔是純粹的 **編譯期常數聚合** 設計。把「分散在多個系統中都需要的同一批數值」集中至一個標頭，實現了 **DRY（Don't Repeat Yourself）** 原則的最基礎應用。`constexpr` 讓這些值在編譯時完全解析，無執行期開銷，且無法意外被修改。注解中對「跨檔案同步約束」的明確說明（更新 PNG 需同步 `kSize`、修改 hit-box 需同步 `kPlayerWidth/Height`）是良好的維護性文件實踐。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/WorldConfig.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/WorldConfig.h) · [← 全檔索引](../files-index.md)
