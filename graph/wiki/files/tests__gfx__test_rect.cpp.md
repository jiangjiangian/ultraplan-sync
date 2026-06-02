---
id: file:tests/gfx/test_rect.cpp
type: test
path: tests/gfx/test_rect.cpp
domain: tests
bucket: gfx
loc: 55
classes: []
sources: ["tests/gfx/test_rect.cpp"]
---
# `test_rect.cpp`

> **一句定位**：驗證 `Rect` 的預設建構、聚合初始化、`Contains` 的半開區間語意（含左上不含右下），以及 `Intersects` 的對稱重疊判定。

## 職責

此測試以六個 TEST_CASE 覆蓋 `engine/math/Rect` 的核心幾何合約。`Rect` 是 engine 層的矩形資料型別 `{x, y, width, height}`，用於碰撞偵測、觸發區域、相機視口等。測試特別釘住 `Contains` 的**半開區間**語意（`[x, x+w)×[y, y+h)`），這影響碰撞邊界的精確行為。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("Rect：預設建構各欄位為零")`：`constexpr Rect r;` 斷言 x 和 width 為 0（height 類推）。
- `TEST_CASE("Rect：以 {x,y,w,h} 聚合初始化")`：`constexpr Rect r{10,20,30,40}`；驗證 x 和 width。
- `TEST_CASE("Rect::Contains 在點落在矩形內時為真")`：`Rect{10,10,20,20}`，斷言 `(15,15)` 為真、`(5,5)` 和 `(31,15)` 為假。
- `TEST_CASE("Rect::Contains：含左上邊界、不含右下邊界（半開區間）")`：`Rect{0,0,10,10}`，`(0,0)` 包含，`(10,10)` 排除——明確釘住半開語意。
- `TEST_CASE("Rect::Intersects：兩矩形重疊時為真（對稱）")`：`Rect{0,0,10,10}` 與 `{5,5,10,10}` 重疊，斷言 `a.Intersects(b)` 和 `b.Intersects(a)` 均為真。
- `TEST_CASE("Rect::Intersects：兩矩形不相交時為偽")`：分離的矩形 `{0,0,10,10}` 與 `{20,20,5,5}` 均為假。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Rect.h`（受測），`engine/math/Vec2.h`（`Contains` 的參數型別）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純幾何資料型別測試）

## OO 概念與設計重點

半開區間的明確測試案例對整個碰撞系統至關重要：`Contains` 的語意決定玩家是否「進入建築物」或「碰到觸發物」的邊界條件。對稱性測試（`a.Intersects(b) == b.Intersects(a)`）則防止程式員在呼叫方向上產生不一致行為。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_rect.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_rect.cpp) · [← 全檔索引](../files-index.md)
