---
id: file:tests/gfx/test_walk_cycle.cpp
type: test
path: tests/gfx/test_walk_cycle.cpp
domain: tests
bucket: gfx
loc: 63
classes: []
sources: ["tests/gfx/test_walk_cycle.cpp"]
---
# `test_walk_cycle.cpp`

> **一句定位**：驗證 Pipoya 步行貼圖的欄位選擇函式（`WalkColumn`）和面向對應行（`WalkRowForFacing`）——這些是 `NPC::Render` 依賴的純函式，在無 GL 環境下釘住格位選擇邏輯。

## 職責

此測試以五個 TEST_CASE 覆蓋 `game/gfx/WalkCycle.h` 的兩個核心函式。它們是 NPC 和 Player 走路動畫的貼圖格位計算，因為貼圖本身的 `DrawSprite` 需要 GL context，所以無法直接在 ctest 中測試，但其格位選擇邏輯是純函式，可完整測試。

`WalkColumn(step)` 把步數（0-3）對映到 Pipoya 圖格的欄位：idle(1) → left_foot(0) → idle(1) → right_foot(2)，在兩個跨步格之間夾 idle 格讓踏步看起來自然。函式對任意整數（含負數）均正確環繞。

`WalkRowForFacing(velocity)` 把速度向量對映到面向列：下(0)、左(1)、右(2)、上(3)，以絕對值較大的主軸決定方向，相等時偏向垂直（行走中軸決策）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("WalkColumn 四步循環為 idle -> left -> idle -> right 並正確環繞")`：驗證 step=0,1,2,3,4,7,-1,-4 的正確欄位，含正/負數環繞。
- `TEST_CASE("WalkColumn 與標準的 kWalkColumns 表一致")`：對 step=0..3 驗證與常數表 `kWalkColumns` 逐項相等。
- `TEST_CASE("WalkRowForFacing 將四個基本方向對應到列（0=下..3=上）")`：正交四方向的精確對映。
- `TEST_CASE("WalkRowForFacing：絕對值較大的主軸決定方向，相等時偏向垂直")`：`(-3,1)`→左、`(3,1)`→右、`(1,1)`→下（平手偏垂直）、`(1,-1)`→上。
- `TEST_CASE("WalkRowForFacing：零向量時靜止面向下（列 0）")`：`(0,0)` → 0。

## 相依與在架構中的位置

- **#include（往外）**：`game/gfx/WalkCycle.h`（提供 `WalkColumn`、`WalkRowForFacing`、`kWalkColumns`），`engine/math/Vec2.h`（面向向量型別）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純函式邏輯測試）

## OO 概念與設計重點

「測試純函式而非 GL 繪製」是 [DIP Renderer 架構](../concepts/arch-dip-renderer.md) 的自然延伸：把算術邏輯從算繪呼叫中抽離，讓這部分可在無 GL 環境下測試。平手時「偏向垂直」的決策對俯視角 RPG 很重要，因為玩家在對角線移動時通常期望看到垂直朝向（上/下）而非水平（左/右）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_walk_cycle.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_walk_cycle.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
