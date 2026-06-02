---
id: file:include/game/gfx/WalkCycle.h
type: header
path: include/game/gfx/WalkCycle.h
domain: game
bucket: gfx
loc: 61
classes: []
sources: ["include/game/gfx/WalkCycle.h"]
---
# `WalkCycle.h`

> **一句定位**：純標頭的 Pipoya 行走圖格運算，Player 與 NPC 共用的「96×128 圖中該貼哪一格 32×32」唯一受測真實來源。

## 職責

`WalkCycle.h` 提供兩個 `constexpr` 函式與相關常數，負責 Pipoya 96×128 sprite 圖集的行走影格與朝向列計算。

**Pipoya 圖集規格**：3 欄（行走影格：左腳/idle/右腳）× 4 列（朝向：下/左/右/上），每格 32×32。行走序列為 idle(1) → 左腳(0) → idle(1) → 右腳(2)，4 步計數讓兩個跨步影格在 idle 影格間來回，呈現自然的踏步。常數：`kPipoyaCell=32`、`kWalkFrameDuration=0.15f`（每步秒數）、`kWalkColumns={1,0,1,2}`。

**`WalkColumn(int s)`**：取動畫步進 `s` 對應的影格欄位。步進 0 是 idle 欄（靜止時顯示 idle 姿勢）；內部以 `((s%4)+4)%4` 安全摺回 `[0,4)`（負數安全）並查 `kWalkColumns`。

**`WalkRowForFacing(Vec2 facing)`**：由朝向向量取 Pipoya 朝向列（0=下、1=左、2=右、3=上）。取絕對值較大的軸為主；相等時偏向垂直軸（完美對角面向上/下）；零向量回傳 0（朝下，標準「面向鏡頭」靜止）。

Player 與 NPC 共用這兩個函式，確保人群漫步 NPC 與玩家的步態完全一致。函式不碰 raylib/GL/模擬狀態，使圖格選擇可在無 GL context 的無頭測試中驗證（`tests/entities/test_npc_animation.cpp`、`tests/gfx/test_walk_cycle.cpp`）。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline constexpr int kPipoyaCell = 32`**：Pipoya 單格像素數。
- **`inline constexpr float kWalkFrameDuration = 0.15f`**：每步秒數。
- **`inline constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2}`**：4 步計數對應的欄位序列。
- **`constexpr int WalkColumn(int s) noexcept`**：取步進 `s` 的影格欄位（`[[nodiscard]]`）；負數安全。
- **`constexpr int WalkRowForFacing(Vec2 facing) noexcept`**：由朝向向量取 Pipoya 列（0..3）（`[[nodiscard]]`）；零向量回傳 0。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Vec2.h`（`WalkRowForFacing` 參數型別）；`<array>`、`<cmath>`、`<cstddef>`（STL）。
- **被誰使用（往內）**：`src/game/entities/NPC.cpp`（`NPC::CurrentRenderCell` 與 `Render`）；`tests/entities/test_npc_animation.cpp`（NPC 動畫狀態測試）、`tests/gfx/test_walk_cycle.cpp`（純函式單元測試）。
- **繼承 / 實作 / 體現**：—（純函式 + 常數，無類別）。
- **每幀管線 / MVC 角色**：View 層計算工具（由 Render 路徑使用）。Player 的 `Render` 與 NPC 的 `CurrentRenderCell`/`Render` 均呼叫這兩個函式，將動畫步進 + 朝向轉換為 Pipoya 格坐標，再交給 `IRenderer::DrawSprite` 切出對應 UV 矩形。

## OO 概念與設計重點

`WalkCycle.h` 與 `Bounds.h`、`SpriteStrip.h` 同屬「純 constexpr 幾何工具」家族，體現 header-only 純函式的最佳設計：零 raylib 相依、`constexpr` 可編譯期求值（雖然這裡主要價值在無頭測試而非編譯期）、`noexcept` 承諾。

「Player 與 NPC 共用同一套運算」消除了兩者步態不同步的風險——如果分別實作，維護時可能漂移。`((s%4)+4)%4` 的負數安全摺回是防禦性細節：`animStep_` 在設計上只會是非負整數，但防禦性處理讓函式在任意整數輸入下安全。`WalkRowForFacing` 的「垂直軸偏好」（相等時偏向 y 軸）是美術決策：完美對角走動時面向上/下比面向左/右更自然。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/WalkCycle.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/WalkCycle.h) · [← 全檔索引](../files-index.md)
