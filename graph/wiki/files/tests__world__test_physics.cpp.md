---
id: "file:tests/world/test_physics.cpp"
type: test
path: tests/world/test_physics.cpp
domain: tests
bucket: world
loc: 123
classes: []
sources: ["tests/world/test_physics.cpp"]
---
# `test_physics.cpp`

> **一句定位**：驗證 `physics::ResolveMove` 的逐軸碰撞解析——無 collider 接受目標、遠處不影響、正面撞牆、對角滑動、多 collider 任一擋住，以及起點重疊時的脫困條款。

## 職責

本測試以精確的幾何 fixture 驗證 `ResolveMove` 的逐軸碰撞解析算法（不含地形遮罩，地形測試見 `test_collision_mask.cpp`）。

所有測試以 `doctest::Approx` 驗證浮點結果，使用玩家大小（24×24 px）為 size fixture。

**測試案例**（共 7 個）：
1. **無 collider → 接受目標**：`(100,100)` 走到 `(120,130)`，無任何障礙，out = desired。
2. **遠處 collider 不影響**：collider 在 (500,500)，玩家走到 (110,110)，out = desired。
3. **正面朝東撞牆 → X 被擋**：牆在 x=130，玩家 prev.x=100（右緣 124 淨空）走到 desired.x=110（右緣 134 觸牆）→ `out.x=100`，`out.y=100`（Y 增量為零）。
4. **對角撞牆角 → 沿自由軸滑動**（兩個 SUBCASE）：X 被擋→Y 自由（沿東牆向下滑動）；Y 被擋→X 自由（沿南牆向東滑動）。
5. **多個 collider，任一擋住即擋住**：三個 collider 中第二個擋住 X，`out.x = 100`（不論其他兩個在遠處）。
6. **NPC 大小的 collider 也會擋住玩家**：24×24 NPC 擋住同等大小玩家的 X 步。
7. **起點已與 collider 重疊 → 脫困**：prev 完全被牆包圍時，兩軸測試都失敗，脫困條款允許移動到 desired（不卡死）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::physics::ResolveMove(Vec2 prev, Vec2 desired, Vec2 size, vector<Rect> colliders, CollisionMask* mask = nullptr)` — 被測函式（此檔只傳 nullptr 或無 mask 參數）。
- `Vec2` / `Rect` — 被用到的幾何型別。
- 物理常數隱含：玩家大小 `{24, 24}`；碰撞寬度由 `size` 決定。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/Physics.h`（受測主體，含 `ResolveMove`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Movement → Collision 階段的單元測試；`ResolveMove` 在 `MovementSystem` / `GameController` 每幀調用。

## OO 概念與設計重點

純 doctest 單元測試，每個案例都有精確計算的座標以消除「測試可能因恰好不觸牆而通過」的假陽性風險。「脫困條款」是對生成時疊加（NPC 碰撞箱與玩家重疊）的防禦性設計，體現了「永不讓玩家卡死」的遊玩品質不變量。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/world/test_physics.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/world/test_physics.cpp) · [← 全檔索引](../files-index.md)
