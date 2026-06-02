---
id: file:include/game/gfx/SpriteStrip.h
type: header
path: include/game/gfx/SpriteStrip.h
domain: game
bucket: gfx
loc: 125
classes: [DecorationDef]
sources: ["include/game/gfx/SpriteStrip.h"]
---
# `SpriteStrip.h`

> **一句定位**：純標頭的 sprite strip 動畫運算工具，提供乒乓影格索引（`FrameAt`）、來源矩形計算（`StripSourceRect`）、`DecorationDef` 結構與目標矩形計算（`DecorationDestRect`）——全部無 raylib、無 GL。

## 職責

`SpriteStrip.h` 封裝「水平 PNG strip 在某時刻應顯示哪一格、以及其 UV 矩形」的純幾何運算，與 `WalkCycle.h` 同類型：所有函式均為 `constexpr`/`inline`/純函式，可在無 GL context 的無頭測試中執行。

**乒乓影格索引 `FrameAt(t, n, fps)`**：實作「先正後逆」三角波（週期 `2*(n-1)` tick），讓動畫裝飾以緩和的放大縮小呼吸感循環，而非末格直接跳回首格的硬切。退化輸入（`n<=1`、`fps<=0`、非有限 `t`、負 `t`）全部安全處理（回傳 0 或以 `floor`+mod 化負為非負）。

**來源矩形 `StripSourceRect(index, frameCount, texW, texH)`**：給定影格索引與 texture 尺寸，回傳 `DrawTexturePro` 需要的來源子矩形（`Rect{index*frameW, 0, frameW, texH}`）。`frameCount<=0` 退化為整張 texture。

**`DecorationDef`**：純資料 struct（`chapter`、`center`、`stripPath`、`frameCount`、`drawScale`、`fps`），無 texture handle。View 依索引配對已載入的 texture；World 看不到這些定義。

**目標矩形 `DecorationDestRect(d, texW, texH)`**：保留影格長寬比，把「較長邊」縮放到 `drawScale` 像素，以 `center` 為中心回傳螢幕空間矩形，使乒乓縮放圍繞錨點對稱脈動。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline int FrameAt(double t, int n, double fps) noexcept`**：乒乓影格索引，週期 `2*(n-1)`；退化輸入安全。
- **`inline Rect StripSourceRect(int index, int frameCount, int texW, int texH) noexcept`**：第 `index` 格的來源子矩形。
- **`struct DecorationDef`**：`chapter`（`SemesterState`）、`center`（`Vec2`）、`stripPath`（`const char*`）、`frameCount`（`int`）、`drawScale`（`float`）、`fps`（`double`）。
- **`inline Rect DecorationDestRect(const DecorationDef& d, int texW, int texH) noexcept`**：保留長寬比、以 center 為中心的螢幕目標矩形。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/math/Rect.h`（矩形型別）、`include/engine/math/Vec2.h`（座標型別）、`include/game/state/SemesterState.h`（`DecorationDef::chapter` 型別）；`<cmath>`（`std::isfinite`、`std::floor`）。
- **被誰使用（往內）**：`include/game/gfx/Decorations.h`（定義 `kDecorations`）、`src/ui/View.cpp`（呼叫 `FrameAt`/`StripSourceRect`/`DecorationDestRect` 繪製裝飾）、`tests/gfx/test_sprite_strip.cpp`（無頭單元測試）。
- **繼承 / 實作 / 體現**：—（無繼承，純函式 + POD struct）。
- **每幀管線 / MVC 角色**：View 層純計算工具。View 每幀讀取 render clock（時間累積），以 `FrameAt` 計算影格、`StripSourceRect` 取 UV、`DecorationDestRect` 取目標位置，最後呼叫 `IRenderer::DrawSprite`。完全不進入 Model 或 Controller。

## OO 概念與設計重點

`SpriteStrip.h` 是 header-only 幾何計算庫的典型形式，與 `WalkCycle.h`、`Bounds.h` 同一風格：純函式 + `inline`/`constexpr`，無 raylib 相依，使動畫邏輯可無頭測試。

`FrameAt` 的乒乓實作特別值得注意：使用 `floor()` 而非截斷處理負 `t`，確保負輸入在 `t==0` 處單調遞增（即便異常時鐘仍不產生負索引）；`mod + 補週期` 解決 C++ `%` 對負數的 UB 問題。這些防禦性細節讓函式在任意輸入下安全，是測試驅動開發的常見品質基準。`DecorationDef` 的 `frameCount` 隨 def 攜帶（不解析檔名）是「最簡單而穩健的約定」，避免命名約定漂移。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/SpriteStrip.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/SpriteStrip.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
