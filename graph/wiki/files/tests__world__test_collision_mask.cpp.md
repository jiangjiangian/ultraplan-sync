---
id: "file:tests/world/test_collision_mask.cpp"
type: test
path: tests/world/test_collision_mask.cpp
domain: tests
bucket: world
loc: 110
classes: []
sources: ["tests/world/test_collision_mask.cpp"]
---
# `test_collision_mask.cpp`

> **一句定位**：驗證 `CollisionMask` 像素級地形遮罩的空遮罩、Solid 查詢、BlockedBox 偵測，以及 `ResolveMove` 在地形遮罩下的逐軸阻擋/滑動行為。

## 職責

本測試補足了 `test_physics.cpp` 的動態矩形碰撞，覆蓋地形像素遮罩（靜態世界幾何）的行為。

測試 fixture `MakeWallMask()`：64×64 遮罩，欄 [30,40) 每一列都有一道 10px 厚的實心垂直牆，足以被 4px 腳印掃描捕捉。

**CollisionMask 查詢**：
- `Empty()` 遮罩：`Solid(x,y)` 永遠為 false；`BlockedBox` 永遠為 false。
- `Solid(30,10)` / `Solid(39,63)` 回傳 true；`Solid(29,10)` / `Solid(40,10)` 回傳 false；出界取樣（-5,-5 / 99999,0）夾到邊界，回傳 false。
- `BlockedBox(0,0,24,24)` 牆西側：false；`(20,0,24,24)` 跨在牆上：true；`(32,0,4,4)` 完全在牆內：true；`(44,0,16,16)` 牆東側：false。

**ResolveMove 與地形遮罩**：
- `mask=nullptr` 時維持只看矩形的舊行為（向後相容）。
- 地形擋住 X 步（`prev.x=5`，`desired.x=10`，右緣觸牆）；Y 軸（此欄無牆）可滑動：`out.x=5.0`，`out.y=18.0`。
- 完全在牆東側的移動暢通無阻。
- 動態矩形與地形遮罩同時作用：Y 軸無牆但有動態 collider 擋住，`out.y=0.0`。

## 關鍵內容（類別 / 函式 / 資料）

- `MakeWallMask()`：建立含垂直牆的 64×64 `CollisionMask` fixture。
- `CollisionMask::Empty()` / `Solid(int x, int y)` / `BlockedBox(float, float, float, float)` — 被測查詢 API。
- `nccu::physics::ResolveMove(prev, desired, size, colliders, mask)` — 被測（第 5 個 mask 參數新增）。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/CollisionMask.h`（受測主體）、`game/world/Physics.h`（`ResolveMove`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Collision 系統的單元測試，對應每幀管線中 Movement → Collision 的地形階段。

## OO 概念與設計重點

純 doctest 單元測試，以合成 fixture 隔離被測行為。`mask=nullptr` 的向後相容測試確保既有動態碰撞測試與呼叫端不受新參數影響——體現了開閉原則（對擴展開放，對既有行為封閉）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/world/test_collision_mask.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/world/test_collision_mask.cpp) · [← 全檔索引](../files-index.md)
