---
id: file:include/game/gfx/Decorations.h
type: header
path: include/game/gfx/Decorations.h
domain: game
bucket: gfx
loc: 69
classes: []
sources: ["include/game/gfx/Decorations.h"]
---
# `Decorations.h`

> **一句定位**：已擺放的氛圍裝飾表——兩個章節專屬的純裝飾動畫物件（Ch2 廣場雕像、Ch3 操場貓），屬 View 端資料，不進入 World 或序列化輸出。

## 職責

`Decorations.h` 定義 `kDecorations` 陣列（`inline constexpr std::array<DecorationDef, 2>`），包含兩個氛圍裝飾：

- **chiikawa 雕像**（Ch2 期中考）：位於羅馬廣場 `kRomaPlazaStatue`（1088,1040），17 影格 / 6fps / 顯示尺寸 80px，以乒乓縮放給出「廣場紀念碑」的呼吸感。座標刻意下移（非廣場幾何中心 960，而是 1040），讀來像「學霸靠在雕像下休息」。
- **操場貓**（Ch3 校慶）：位於 `kSportsCatPos`（1530,kSportsTrackCy），24 影格 / 8fps / 顯示尺寸 28px（「小小的」），同樣乒乓呼吸。座標西移避開綜合院館覆蓋（原跑道中心 1694 落在建築佔地內）。

兩個裝飾皆為 View 端資料：不是 `GameObject`、不在 `World::Objects()` 中、不影響碰撞或序列化輸出——整局位元一致，有無美術資産不影響遊戲結果。`stripPath` 缺檔時完全不畫（空資源路徑守衛）。兩者的 strip PNG 為第三方同人圖，於 CREDITS.md 標註、不提交入庫。

還定義了兩個 `inline constexpr` 座標常數：`kRomaPlazaStatue` 與 `kSportsCatPos`，並附詳細注解說明座標選取的幾何理由。

## 關鍵內容（類別 / 函式 / 資料）

- **`inline constexpr Vec2 kRomaPlazaStatue{1088.0f, 1040.0f}`**：廣場雕像中心，下移至學霸頭頂附近（非廣場幾何中心）。
- **`inline constexpr Vec2 kSportsCatPos{1530.0f, kSportsTrackCy}`**：操場貓繪製中心，西移避開綜合院館遮擋。
- **`inline constexpr std::array<DecorationDef, 2> kDecorations`**：兩個裝飾的完整定義表（chapter、center、stripPath、frameCount、drawScale、fps）。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/gfx/SpriteStrip.h`（`DecorationDef` 型別、`FrameAt`/`StripSourceRect` 函式）、`include/engine/math/Vec2.h`（座標型別）、`include/game/quest/Chapter3Quest.h`（`kSportsTrackCy` 常數）、`include/game/state/SemesterState.h`（`SemesterState::Chapter2_Midterms`/`Chapter3_SportsDay`）。
- **被誰使用（往內）**：`include/game/world/TexturePreload.h`（預載入 strip texture）、`src/ui/View.cpp`（依 FSM 章節繪製裝飾）、`tests/gfx/test_sprite_strip.cpp`。
- **繼承 / 實作 / 體現**：—（純常數資料，無類別）。
- **每幀管線 / MVC 角色**：純 View 端資料。View 在每幀的 render clock 讀 `kDecorations`，依當前 `SemesterState` 篩選並以 `FrameAt` 計算影格索引，呼叫 `IRenderer::DrawSprite` 繪製。完全不在 Model 管線中。

## OO 概念與設計重點

`Decorations.h` 是「View 端編譯期常數資料」的設計：`inline constexpr` 確保無執行時開銷且只有一份實例；資料化（`DecorationDef` struct + 陣列）讓 View 以迴圈統一處理，而不需 if/switch。座標選取附詳細注解（幾何中心 vs 視覺合理中心的取捨），是「程式碼即文件」的良好範例。

對 `kSportsTrackCy` 的跨包依賴（引用 `Chapter3Quest.h`）體現「單一事實來源」——操場跑道中心座標只在 Chapter3Quest 定義一次，裝飾座標引用它，避免漂移。裝飾明確不進入 `World` 的設計維持了模擬層與視覺層的 [MVC](../concepts/arch-mvc.md) 分離。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/gfx/Decorations.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/gfx/Decorations.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
