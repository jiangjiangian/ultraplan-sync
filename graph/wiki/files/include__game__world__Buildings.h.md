---
id: file:include/game/world/Buildings.h
type: header
path: include/game/world/Buildings.h
domain: game
bucket: world
loc: 65
classes: [Building]
sources: ["include/game/world/Buildings.h"]
---
# `Buildings.h`

> **一句定位**：政大山下校園 26 棟建築的靜態資料表——名稱、進入觸發矩形與貼圖鏡像旗標，由 Tiled 匯出工具自動產生。

## 職責

本標頭是校園建築布局的「單一真實來源」，以 `inline constexpr std::array<Building, 26> kAll` 的形式將所有建築的名稱、進入觸發矩形（`triggerRect`）與 Tiled 鏡像旗標（`flipX`、`flipY`）烘焙為編譯期常數。

資料由 `tiled_to_world.py` 工具從 Tiled 地圖匯出，直接以 `{x, y, width, height}` 初始化。需要更新建築位置時只需重跑工具並貼回 `kAll` 陣列即可，不需改動消費端邏輯。

需要特別注意的是，`triggerRect` 僅用於 `BuildingTracker` 的進入觸發判定，**與實體碰撞無關**——碰撞形狀由像素級可走遮罩 `CollisionMask` 描述，兩者分離，避免觸發區與碰撞區互相耦合。羅馬廣場不在此表中（其開放廣場已烘焙進底圖，不需進入事件）。

## 關鍵內容（類別 / 函式 / 資料）

- `buildings::Building`（struct）：單棟建築的純資料記錄：
  - `name`（`std::string_view`）：繁體中文建築名稱，也用作貼圖路徑的組成部分。
  - `triggerRect`（`nccu::engine::math::Rect`）：進入觸發矩形，供 `BuildingTracker::NearestContaining` 判定。
  - `flipX`（`bool`，預設 `false`）、`flipY`（`bool`，預設 `false`）：對應 Tiled 的水平／垂直鏡像旗標，供 View 翻轉貼圖。
- `buildings::kAll`（`inline constexpr std::array<Building, 26>`）：26 棟建築完整資料，含大勇樓、大仁樓、大智樓、學思樓、商學院、四維堂、果夫樓、志希樓、校友服務中心、樂活館、樂活小舖、游泳館、綜合院館、法學院、研究大樓、井塘樓、新聞館、集英樓、資訊大樓、風雩樓、風雩走廊、行政大樓、體育館、操場、正門、中正圖書館。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Rect.h`（`Rect` 型別，定義觸發矩形）；`<array>`、`<string_view>`（標準庫）。
- **被誰使用（往內）**：`include/game/world/BuildingTracker.h`（消費 `kAll` 做進入判定）；`include/game/world/TexturePreload.h`（遍歷 `kAll` 預熱建築貼圖）；`src/ui/View.cpp`（遍歷 `kAll` 繪製建築 sprite 並套用 `flipX/flipY`）；`tests/entities/test_rain_survival.cpp`、`tests/ui/test_font_ui_glyphs.cpp`（測試用到建築名稱）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純資料定義，屬於 Model 層（game/world）。View 以唯讀方式遍歷 `kAll` 繪製建築；BuildingTracker 以 `kAll` 做觸發判定；TexturePreload 在進入遊戲前以 `kAll` 預熱貼圖。

## OO 概念與設計重點

本檔是典型的 **編譯期純資料表（constexpr data table）**，無任何方法。以工具自動產生的靜態資料取代手刻的建築定義，確保資料與 Tiled 地圖同步。`flipX` 預設值 `false` 使舊式兩欄（只帶 `name` 與 `triggerRect`）的初始化仍能編譯，是向後相容的設計選擇。觸發矩形與碰撞遮罩的分離遵循 **關注點分離（SoC）**：進入事件由矩形驅動，碰撞解算由像素遮罩驅動，兩個子系統可獨立演化。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/Buildings.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/Buildings.h) · [← 全檔索引](../files-index.md)
