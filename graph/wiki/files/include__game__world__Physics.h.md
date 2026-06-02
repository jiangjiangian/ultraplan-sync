---
id: file:include/game/world/Physics.h
type: header
path: include/game/world/Physics.h
domain: game
bucket: world
loc: 57
classes: []
sources: ["include/game/world/Physics.h"]
---
# `Physics.h`

> **一句定位**：軸分離的 AABB 移動解算器，header-only 純函式，同時處理動態碰撞體（其他角色）與靜態地形遮罩的滑牆判定。

## 職責

`physics::ResolveMove` 實現了 JRPG 常見的「沿牆滑行」移動手感：斜向走入牆角時，僅在未被擋住的軸上繼續移動，另一軸停在前一幀位置。

解算策略分兩軸分別嘗試：先單獨測試 X 位移（`overlapsAny(desired.x, prev.y)`），若碰到任何障礙則 X 維持 `prev.x`；再從 X 解算後的位置嘗試 Y 位移。每次測試同時涵蓋兩種障礙：動態碰撞體（`colliders` 向量，即其他角色的 AABB）與靜態地形遮罩（`CollisionMask`）；後者以 `nullptr` 傳入代表跳過（用於 NPC 路徑或單元測試）。

特別設計了**脫困模式**：若 `prev` 本身已與某障礙重疊（出生在碰撞體上、NPC 走進玩家等），直接放行到 `desired`——假定下一幀的 `prev` 已淨空後正常阻擋將恢復。若不這樣處理，`overlapsAny(prev.x, prev.y)` 始終為真，X 與 Y 測試都會失敗，玩家永遠卡死。

整個實作是 `inline` 函式，無外部狀態，不依賴 raylib，可直接在單元測試中驗證。

## 關鍵內容（類別 / 函式 / 資料）

- `physics::ResolveMove(prev, desired, playerSize, colliders, mask = nullptr) -> Vec2`（`inline`）：
  - `prev`：玩家本幀更新前的左上角座標。
  - `desired`：玩家想移動到的座標。
  - `playerSize`：AABB 尺寸（`Vec2`）。
  - `colliders`：本幀的動態碰撞體清單（`const std::vector<Rect>&`），呼叫端每幀重建。
  - `mask`：可選靜態地形遮罩（`const CollisionMask*`），`nullptr` 跳過地形碰撞。
  - 內部 lambda `overlapsAny(x, y)`：在給定位置建出 AABB，與 `colliders` 逐一測試 `Intersects`，並查詢 `mask->BlockedBox`。
  - 脫困：若 `overlapsAny(prev.x, prev.y)` 為真直接返回 `desired`。
  - 正常流：先解 X，再從 X 結果解 Y。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Vec2.h`、`engine/math/Rect.h`（座標與 AABB 型別）；`game/world/CollisionMask.h`（靜態地形碰撞）；`<vector>`。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（玩家移動解算）；`src/game/controller/SimSystems.cpp`（系統管線中的移動系統）；`src/game/entities/NPC.cpp`（NPC 自主移動解算）；`tests/world/test_collision_mask.cpp`、`tests/world/test_physics.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 Movement 階段（每幀管線 Survival → **Movement** → Collision → Spawn → …），是 Controller 呼叫的 game 層物理工具，自身無狀態。

## OO 概念與設計重點

`ResolveMove` 是 **header-only 無狀態純函式**，不依賴任何全域物件，所有輸入都以參數傳入，可在無 GL 環境中精確單元測試。`mask` 以 `nullptr` 可選的設計允許 NPC 移動（不需地形碰撞）與玩家移動（需要地形碰撞）共用同一函式，體現了 **OCP（開放封閉）** 的輕量應用。脫困模式體現了對**邊界條件的明確防禦性處理**——程式碼直接說明「為何」這樣做（避免卡死），而非只說「做什麼」。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/Physics.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Physics.h) · [← 全檔索引](../files-index.md)
