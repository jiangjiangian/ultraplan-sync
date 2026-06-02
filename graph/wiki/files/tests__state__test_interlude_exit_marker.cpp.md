---
id: file:tests/state/test_interlude_exit_marker.cpp
type: test
path: tests/state/test_interlude_exit_marker.cpp
domain: tests
bucket: state
loc: 167
classes: [Spy, RectCall, TextCall]
sources: ["tests/state/test_interlude_exit_marker.cpp"]
---
# `test_interlude_exit_marker.cpp`

> **一句定位**：以攔截型 `Spy` 渲染器驗證幕間出口地標（金色虛線）的佈局計算、動畫相位、雙通道繪製（陰影+金色本體）與顏色契約，完全不呼叫 raylib。

## 職責

此測試檔是 View 層測試的典型範例：透過繼承 `IRenderer` 的 `Spy` 攔截器，在無 GL 環境下驗證 `DrawInterludeExitMarker` 和 `LayoutInterludeExitMarker` 的行為。共六個 TEST_CASE：

1. **佈局產生虛線段**：`LayoutInterludeExitMarker(0.0f)` 返回非空的 `dashes`；每段都落在出口區北緣（`y == kInterludeExitMinY`）、走廊 x 範圍內、線寬 ≤ `kInterludeMarkerDashLen`。

2. **覆蓋整條走廊**：`dashes.size() >= 20`（寬鬆下界，走廊寬 ~1800px，40px+20px 週期約 30 段）。

3. **相位週期性**：相位 0 與相位等於一個完整週期（`kInterludeMarkerDashLen + kInterludeMarkerGapLen`）產生相同的虛線清單。

4. **相位偏移效果**：相位 10.0f 時，未被裁切的虛線 x 位移恰好 +10px（流動效果驗證）。

5. **每段虛線繪製兩個矩形**：`Spy.rects.size() == dashes.size() * 2`（陰影 + 金色本體），`texts` 和 `sprites` 為零。

6. **顏色契約**：陰影為 RGB(0,0,0,α<255)；金色本體為 RGB(255,200,61,255)（`#FFC83D`）；陰影相對本體往東南偏移 2px。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Spy : nccu::engine::render::IRenderer`：測試替身，記錄 `DrawRect`/`DrawSprite`/`DrawText` 的呼叫。
- `struct RectCall { Rect r; Color c; }`：矩形記錄。
- `struct TextCall { string s; Vec2 pos; int size; Color c; }`：文字記錄。
- `nccu::LayoutInterludeExitMarker(float phase)`：佈局計算（純算術，可測試）。
- `nccu::DrawInterludeExitMarker(IRenderer&, float phase)`：驅動渲染器的繪製函式。
- 常數：`kInterludeExitMinY`、`kInterludeExitMinX`、`kInterludeExitMaxX`、`kInterludeMarkerThickness`、`kInterludeMarkerDashLen`、`kInterludeMarkerGapLen`。

## 相依與在架構中的位置

- **#include（往外）**：`InterludeExitMarker.h`（佈局+繪製函式及常數）、`InterludeExit.h`（`kInterludeExitMinY`）、`IRenderer.h`（被繼承）。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：`Spy` 繼承 `IRenderer`（`include/engine/render/IRenderer.h`）。
- **每幀管線 / MVC 角色**：測試 View 層（`DrawInterludeExitMarker`），但透過 `IRenderer` 抽象隔離 raylib，對應 [DIP 渲染器架構](../concepts/arch-dip-renderer.md)。

## OO 概念與設計重點

`Spy` 是 [DIP 渲染器架構](../concepts/arch-dip-renderer.md)的測試應用：`IRenderer` 是抽象，`Spy` 是測試替身，讓 View 函式可在無 GL 環境下被精確驗證。`LayoutInterludeExitMarker` 與 `DrawInterludeExitMarker` 的分離（「計算」與「繪製」分開）使佈局邏輯可獨立測試。顏色契約固定（`#FFC83D`）確保地標與任務指示器的視覺調色盤一致性。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_interlude_exit_marker.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_interlude_exit_marker.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
