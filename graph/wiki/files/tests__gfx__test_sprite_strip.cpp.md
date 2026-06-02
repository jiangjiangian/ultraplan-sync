---
id: file:tests/gfx/test_sprite_strip.cpp
type: test
path: tests/gfx/test_sprite_strip.cpp
domain: tests
bucket: gfx
loc: 176
classes: []
sources: ["tests/gfx/test_sprite_strip.cpp"]
---
# `test_sprite_strip.cpp`

> **一句定位**：驗證精靈圖條的逐格計算——`FrameAt` 的 ping-pong 動畫序列（含退化輸入安全性）、`StripSourceRect` 的水平切格、`DecorationDestRect` 的等比置中縮放，以及 `kDecorations` 表的具體擺放位置。

## 職責

此測試以九個 TEST_CASE 覆蓋 `game/gfx/SpriteStrip.h` 與 `game/gfx/Decorations.h` 的純計算邏輯。這些函式是 View 層顯示動畫 NPC 與場景裝飾物的底層算術，無需 GL context 即可完整測試。

`FrameAt(t, n, fps)` 實作 ping-pong（來回三角波）動畫：週期為 `2*(n-1)` 刻度，頂點 `n-1` 每週期恰好碰到一次，退化輸入（n≤1、fps≤0、NaN、Inf、負時間）均安全回傳第 0 格。

`kDecorations` 表的測試釘住了兩個裝飾物的精確座標，帶有詳細的擺放理由：chiikawa 在 Ch2 學霸附近 (1088, 1040)，貓在 Ch3 綜院西側開闊跑道 (1530, kSportsTrackCy)，避免被建築物遮擋。

## 關鍵內容（類別 / 函式 / 資料）

- `Sample(n, ticks)`：呼叫 `FrameAt` 整數刻度 `ticks` 次，收集動畫索引序列。
- `TEST_CASE("FrameAt：n=4 時產生來回三角序列 0,1,2,3,2,1,0,1,...")`：14 刻度的完整序列比對。
- `TEST_CASE("FrameAt：頂點為 n-1，每個週期恰好碰到一次")`：測試 t=3 和 t=9（3+6）均回傳頂點 3。
- `TEST_CASE("FrameAt：長時間取樣下索引永遠落在 [0, n-1]")`：n=2..8 各跑 500 刻度，斷言範圍。
- `TEST_CASE("FrameAt：兩格圖條來回 0,1,0,1,...（週期 2）")`：n=2 的特殊最小情況。
- `TEST_CASE("FrameAt：退化輸入皆安全且為全函式，一律回傳第 0 格")`：n=1、0、-3；fps=0、-1；NaN；Inf。
- `TEST_CASE("FrameAt：負時間也不會產生負索引")`：k=-50..-1 均在 [0,3]。
- `TEST_CASE("FrameAt：fps 控制每格持續幾個刻度")`：fps=6 時 t=1/6 進入第 1 格。
- `TEST_CASE("StripSourceRect：由左至右切割水平圖條")`：256x40/8 格，驗證格 0、格 3、格 7 的 x 偏移和寬度。
- `TEST_CASE("StripSourceRect：frameCount<=0 時退化為整張貼圖")`：輸出寬高等於整張貼圖。
- `TEST_CASE("DecorationDestRect：以錨點置中，較長邊縮放為 drawScale")`：256x64/8 格 → 一格 32x64，較長邊縮放至 40，驗證矩形中點等於錨點。
- `TEST_CASE("DecorationDestRect：寬大於高的格子將寬縮放為 drawScale")`：400x50/4 格 → 100x50，寬縮放 80。
- `TEST_CASE("kDecorations：chiikawa 在學霸附近、貓在綜院西側")`：釘住兩個裝飾物的精確座標、drawScale 大小與 frameCount。

## 相依與在架構中的位置

- **#include（往外）**：`game/gfx/SpriteStrip.h`（`FrameAt`、`StripSourceRect`、`DecorationDestRect`），`game/gfx/Decorations.h`（`kDecorations`、`DecorationDef`），以及 `kSportsTrackCx/Cy`（由其中一個標頭提供）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 View 計算邏輯測試）

## OO 概念與設計重點

退化輸入的全函式保證（全域定義域、無 UB）對動畫系統至關重要：`FrameAt` 可能在每幀由 NPC::Render 呼叫，任何無效輸入若崩潰都難以調試。NaN 和 Inf 的明確測試案例體現了防禦性設計原則。`kDecorations` 表的座標釘住測試是迴歸保護，防止美術調整意外移動了有充分佈局理由的裝飾物。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_sprite_strip.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_sprite_strip.cpp) · [← 全檔索引](../files-index.md)
