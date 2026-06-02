---
id: file:include/game/state/InterludeExitMarker.h
type: header
path: include/game/state/InterludeExitMarker.h
domain: game
bucket: state
loc: 109
classes: [InterludeExitMarkerDash, InterludeExitMarkerLayout]
sources: ["include/game/state/InterludeExitMarker.h"]
---
# `InterludeExitMarker.h`

> **一句定位**：幕間南側出口的地面金色虛線視覺標記——包含布局計算（`LayoutInterludeExitMarker`）、顏色/尺寸常數，以及透過 `IRenderer` 繪製的 `DrawInterludeExitMarker`，完全不引入 raylib。

## 職責

`InterludeExitMarker.h` 為幕間市集南側出口（`InterludeExit.h` 定義的 `y>=1900` 觸發帶）提供視覺軌：一條橫跨可步行幕間走廊的金色水平虛線，讓玩家在鏡頭框到南側帶時即看到離場路線，彌補原先「看不見的觸發帶」缺少視覺回饋的問題。

架構比照任務給予者指示器：純 View 層（僅透過 `IRenderer` 繪製），不引入 raylib（符合 DIP 渲染抽象層），隨鏡頭移動（View 在其 `CameraScope` 內繪製，線條跟隨世界座標）。布局計算抽成 `LayoutInterludeExitMarker(phase)` 獨立函式，使回歸測試能在無 GL 環境下驗證幾何。

視覺規格：金色 `#FFC83D`（RGBA {255,200,61,255}），外加東南偏移 2 px 的半透明黑色陰影（{0,0,0,140}），使虛線在任何地磚上清晰。虛線長 40 px，間隙 20 px（週期 60 px）。`phase` 參數驅動水平動畫（View 以每幀秒數×速度遞推 phase）；phase=0 為測試用的確定性預設。

`LayoutInterludeExitMarker` 以「從帶西界外一個週期起算，掃到東界為止」的迴圈生成虛線段，並夾鉗各段在帶邊界內。`DrawInterludeExitMarker` 對每段虛線先畫陰影再畫主體（雙趟 `DrawRect`），與任務給予者指示器相同作法。

## 關鍵內容（類別 / 函式 / 資料）

- `struct InterludeExitMarkerDash`：單段虛線的世界座標矩形 `rect`（純幾何）。
- `struct InterludeExitMarkerLayout`：虛線段 `dashes`（`vector<InterludeExitMarkerDash>`），View 逐段繪製。
- `kInterludeMarkerThickness = 4.0f`：虛線粗細（px）。
- `kInterludeMarkerDashLen = 40.0f` / `kInterludeMarkerGapLen = 20.0f`：虛線/間隙長度。
- `kInterludeMarkerGold = {255, 200, 61, 255}`：金色主體色。
- `kInterludeMarkerShadow = {0, 0, 0, 140}`：半透明陰影色。
- `LayoutInterludeExitMarker(float phase = 0.0f) → InterludeExitMarkerLayout`：計算虛線段清單，inline，純幾何無副作用，供測試。
- `DrawInterludeExitMarker(IRenderer& r, float phase = 0.0f)`：呼叫 layout 後對每段先畫陰影再畫主體，inline。

## 相依與在架構中的位置

- **#include（往外）**：`InterludeExit.h`（出口帶邊界常數）、`IRenderer.h`（抽象渲染介面）、`Rect.h`（矩形型別）；標準庫 `<cmath>`（`fmod`）、`<vector>`
- **被誰使用（往內）**：`src/ui/View.cpp`（在 CameraScope 內呼叫 `DrawInterludeExitMarker`）、`tests/state/test_interlude_exit_marker.cpp`（布局幾何單元測試）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層——每幀在 `Interlude_Market` 狀態下由 View 呼叫 `DrawInterludeExitMarker`，繪製在世界座標系（跟隨鏡頭）。

## OO 概念與設計重點

本檔體現了 [DIP 渲染抽象](../concepts/arch-dip-renderer.md)：所有繪製呼叫透過 `IRenderer&` 而非直接呼叫 raylib，標頭本身不引入任何 raylib 標頭，使 `LayoutInterludeExitMarker` 在測試環境中可純粹驗證幾何。`phase` 的 `fmod` 正規化確保長時間運行不溢位，動畫行為定義良好。虛線段的「拆成小基本圖元」設計是測試友好的典型：測試可斷言「帶內至少有一段虛線」，而不必做脆弱的像素比對。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/InterludeExitMarker.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/InterludeExitMarker.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
