---
id: file:tests/gfx/test_vec2.cpp
type: test
path: tests/gfx/test_vec2.cpp
domain: tests
bucket: gfx
loc: 55
classes: []
sources: ["tests/gfx/test_vec2.cpp"]
---
# `test_vec2.cpp`

> **一句定位**：驗證 `Vec2` 二維向量的預設建構（原點）、聚合初始化、算術運算子（加減純量乘）、`Length` 歐幾里得長度，以及 `Normalized` 的安全單位化（零向量回傳零向量）。

## 職責

此測試以五個 TEST_CASE 完整覆蓋 `engine/math/Vec2` 的數學合約。`Vec2` 是整個系統中座標、速度、方向的基礎型別，任何計算錯誤都會靜默傳播。測試特別釘住零向量 `Normalized()` 的安全語意——回傳零向量而非 NaN（除以零保護）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("Vec2：預設建構應為原點 (0,0)")`：`constexpr Vec2 v;` 斷言 x==y==0。
- `TEST_CASE("Vec2：以 {x,y} 聚合初始化")`：`constexpr Vec2 v{3,4}`；驗證兩分量。
- `TEST_CASE("Vec2 算術運算子逐分量運算")`：`a+b`（1+3=4, 2+5=7）、`b-a`（2）、`a*2`（2,4）。
- `TEST_CASE("Vec2::Length 回傳歐幾里得長度")`：`{3,4}.Length() == 5`（3-4-5 直角三角形）。
- `TEST_CASE("Vec2::Normalized 回傳單位向量；零向量則回傳零向量")`：零向量 → (0,0)；(3,4).Normalized() → (0.6, 0.8)。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Vec2.h`（唯一依賴），`<cmath>`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純數學型別測試）

## OO 概念與設計重點

`constexpr` 的使用確認 `Vec2` 是字面量型別，可在編譯期用於常數表達式。零向量 `Normalized()` 的安全語意是防禦性設計的體現，因為 NPC 面向計算（`WalkRowForFacing`）可能收到速度為零的向量。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_vec2.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_vec2.cpp) · [← 全檔索引](../files-index.md)
