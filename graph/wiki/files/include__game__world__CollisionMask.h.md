---
id: file:include/game/world/CollisionMask.h
type: header
path: include/game/world/CollisionMask.h
domain: game
bucket: world
loc: 88
classes: [CollisionMask]
sources: ["include/game/world/CollisionMask.h"]
---
# `CollisionMask.h`

> **一句定位**：烘焙地形的像素級可走遮罩——以每像素一 byte 描述建築牆基、河流與手繪障礙，不依賴 raylib 讓物理解算層可直接調用。

## 職責

`CollisionMask` 是地形碰撞系統的核心資料結構。它以一個 `std::vector<uint8_t>` 儲存整張世界地圖的可走資訊，每個世界像素對應一個 byte（1 = 實心不可走、0 = 可走），支援由任意影像編輯器繪製的 PNG 載入（「非純白且非全透明」即為實心）。

關鍵設計是 **完全不依賴 raylib**：`CollisionMask` 只依賴 `Vec2.h` 與標準庫，因此 `Physics.h` 的 AABB 解算器可在 game/world 層直接使用遮罩，不需引入 gfx/render 標頭。實際的 PNG 載入（需要 raylib）封裝在 `TerrainMask.cpp` 裡的 `nccu::game::gfx::LoadCollisionMask` 中，使模型層對 raylib 保持零依賴。

`BlockedBox()` 以 `kStep = 4.0f` 像素的格點掃描 AABB，永遠包含遠端邊界，確保薄牆不會從取樣間隙漏過。空遮罩（`Width() == 0`）的所有查詢回傳 `false`（可走），作為缺少遮罩 PNG 時的優雅降級——全新 clone 或無頭測試環境下遊戲仍能運行，只是沒有地形碰撞。

優先載入手繪的 `collision_mask.png`（含裝飾物障礙），缺檔時退回工具產生的 `collision_mask_base.png`（僅建築與河流）；兩者都缺則降為空遮罩。

## 關鍵內容（類別 / 函式 / 資料）

- `CollisionMask`（class）：像素級地形可走遮罩。
  - 建構子：`CollisionMask()`（空遮罩）；`CollisionMask(int w, int h, std::vector<uint8_t> solid)`（帶資料初始化）。
  - `Width() / Height() const noexcept`：遮罩尺寸。
  - `Empty() const noexcept`：寬度 ≤ 0 或高度 ≤ 0 時為真。
  - `Solid(int px, int py) const noexcept -> bool`：查詢單一像素是否實心，自動夾限越界座標（不越界存取）。
  - `BlockedBox(float bx, float by, float bw, float bh) const noexcept -> bool`：AABB 版本，以 `kStep=4` 格點掃描，任一像素實心即為真。
  - `BlockedBox(Vec2 pos, Vec2 size) const noexcept -> bool`：`BlockedBox` 的 `Vec2` 多載。
  - 私有：`w_`、`h_`（`int`）、`solid_`（`std::vector<uint8_t>`）。
- `LoadTerrainMask() -> CollisionMask`：全域自由函式，載入正規地形遮罩；實作於 `TerrainMask.cpp`，是唯一允許透過 `gfx::LoadCollisionMask` 接觸 raylib 的路徑。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Vec2.h`（`BlockedBox` 的 `Vec2` 多載參數型別）；`<cstdint>`、`<vector>`（標準庫）。
- **被誰使用（往內）**：`include/game/entities/NPC.h`（NPC 移動時傳入 `mask`）；`include/game/gfx/MaskLoader.h`（gfx 側載入介面）；`include/game/world/Physics.h`（物理解算層的核心依賴）；`include/game/world/World.h`（`World` 成員 `terrainMask_`）；`src/game/world/TerrainMask.cpp`（`LoadTerrainMask` 實作）；多個測試檔。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 Model 層（game/world），是 Collision 階段的底層資料。`Physics::ResolveMove` 每幀接收 `const CollisionMask*`，用於靜態地形碰撞判定。

## OO 概念與設計重點

`CollisionMask` 體現了 **依賴反轉原則（DIP）**：消費端（Physics.h、NPC.h、World.h）只依賴此抽象資料型別，不依賴 raylib；raylib 的接觸被隔離在 `TerrainMask.cpp` 的 `LoadTerrainMask()` 函式後面，符合 [arch-dip-renderer](../concepts/arch-dip-renderer.md) 的精神。`Empty()` 回傳 `false`（全可走）的空遮罩設計是一種 **Null Object** 模式的輕量應用，確保缺少資產時系統仍能優雅執行而非崩潰。`BlockedBox` 的 `kStep=4` 格點掃描是像素精確碰撞與效能之間的明確權衡。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/CollisionMask.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/CollisionMask.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
