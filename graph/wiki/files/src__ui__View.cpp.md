---
id: file:src/ui/View.cpp
type: source
path: src/ui/View.cpp
domain: ui
bucket: 
loc: 368
classes: []
sources: ["src/ui/View.cpp"]
---
# `View.cpp`

> **一句定位**：MVC 的 View 根——`View::Draw` 分派器，統籌世界渲染、HUD、疊層、章節字卡，並組裝相機跟隨、畫家排序（建築 / 裝飾 / 物件 walk-behind）等所有渲染細節。

## 職責

此檔屬於 ui 層，是整個 View 層的核心組裝點，`View` 類別的主要成員函式實作。

**建構式 `View::View(w, h)`**：載入 `worldmap_base.png` 底圖，依 `buildings::kAll` 逐棟嘗試載入 `buildings_3d_trimmed/<name>.png`（跳過 `kBuildingCollisionSkip` 中的兩棟），建立 `BuildingSprite`（含翻轉旗標）；再依 `kDecorations` 載入環境裝飾條 texture。

**`View::Draw(World&)`**：每幀分派器。先呼叫 `UpdateChapterCardTransition(st)` 偵測章節邊界；若結局狀態則呼叫 `RenderEnding` 並提前返回；否則相機跟隨玩家、呼叫 `RenderWorld` → `RenderHud` → `RenderOverlays`。

**`UpdateChapterCardTransition`**：首幀以非章節哨兵觸發開場字卡，後續幀偵測狀態變化觸發對應字卡。純 View 狀態（不發事件、不寫 Model）。

**`RenderEnding`**：清黑底（消除雙緩衝閃爍）→ `EndingFadeAlphaStep` 推進淡入 → 萃取 `EndingSummary` DTO → `DrawEndingCard`。View 在此唯一處持有 World / Player 並構建 DTO，EndingView 本身絕不碰 World。

**`RenderWorld`**：`Renderer{}.Clear(RayWhite)` → `CameraScope` 內：底圖 → `DrawSportsLapTrack`（地面貼花）→ 畫家排序（建築以 `baseY`、裝飾以 `dest底邊`、物件以 `pos.y+kPlayerHeight` 為鍵，`std::sort` + switch `DrawKind`）→ `DrawQuestGiverIndicators`（`!` 圖示）→ 插曲段出口線。裝飾以 `decorationClock_`（`DeltaSeconds()` 累加）推進影格，僅顯示與當前章節匹配的條目。建築 `flipX/flipY` 以負的來源矩形寬 / 高實現鏡像。

**`RenderHud`**：`DrawSportsLapRing` → `DrawStatusPanel` → `DrawRainVignette` → `DrawObjectiveBar`。

**`RenderOverlays`**：Top HudMessage → Bottom HudMessage → `DrawDialog` → 背包疊層（`InventoryOpen` 時建構 `BuildInventoryRows` DTO） → `DrawMenuAffordance` → `DrawPauseMenu` → `DrawHelpOverlay` → `chapterCard_.Step + DrawChapterCard`。

## 關鍵內容（類別 / 函式 / 資料）

- `View::View(int, int)` — 建構式，載入建築 sprite 和裝飾條。
- `View::Draw(const World&)` — 每幀入口；分派到 `UpdateChapterCardTransition` + 四個子渲染函式。
- `View::RenderEnding` / `RenderWorld` / `RenderHud` / `RenderOverlays` — 四個責任分明的渲染子函式。
- `View::UpdateChapterCardTransition(SemesterState)` — 偵測章節邊界，純 View 側觸發字卡。
- `BuildingSprite` — `{texIndex, dest, baseY, flipX, flipY}`，`drawOrder_` 的建築條目。
- `DecorationSprite` — `{defIndex, texture}`，環境裝飾條的紋理持有者。
- `DrawRef` — `{y, DrawKind, obj, index}`，畫家排序用的輕量 struct。
- `drawOrder_` — 每幀重建的排序暫存容器（`reserve` 預先分配）。
- `decorationClock_` — `double` 累加器，驅動裝飾動畫（純 View 狀態）。
- `camera_` — 相機物件，`Follow + ClampToWorld`。
- `endingAlpha_` — 結局淡入透明度（純 View 狀態）。
- `chapterCard_` — `ChapterCardState`（純 View 狀態）。
- `lastSemester_` — `optional<SemesterState>`，用於章節邊界偵測。

## 相依與在架構中的位置

- **#include（往外）**：幾乎所有 ui / game / engine 的顯示介面，詳見 manifest。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `App` 或組裝根每幀呼叫 `Draw`。
- **繼承 / 實作 / 體現**：`View` 是 MVC 的 View 根，只讀 `const World&`。
- **每幀管線 / MVC 角色**：View 根；每幀管線最後執行，讀取 Model 狀態，將所有渲染呼叫送往 `IRenderer`。

## OO 概念與設計重點

`View` 嚴格遵守 [MVC](../concepts/arch-mvc.md)：只讀 `const World&`（結局 DTO 萃取是唯一處）、絕不寫 Model、純 View 狀態（`decorationClock_`、`chapterCard_`、`endingAlpha_`）不進存檔。畫家排序以輕量 `DrawRef` struct + `std::sort` 實現，無虛擬函式呼叫，符合 ISP：`IDrawable::Render` 只對扮演 IDrawable 角色的物件呼叫（[ISP / Roles](../concepts/oo-isp-roles.md)）。[DIP Renderer](../concepts/arch-dip-renderer.md) 確保所有 raylib 呼叫只透過 `IRenderer` 出去，使 View 邏輯可做 headless 測試。`RenderEnding` 的「先清黑底再畫字卡」解決了 raylib 雙緩衝閃爍問題，是底層渲染知識的具體應用。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/View.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/View.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md) · [ISP / Roles](../concepts/oo-isp-roles.md)
