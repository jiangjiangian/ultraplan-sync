---
id: file:include/engine/render/CameraScope.h
type: header
path: include/engine/render/CameraScope.h
domain: engine
bucket: render
loc: 44
classes: [CameraScope]
sources: ["include/engine/render/CameraScope.h"]
---
# `CameraScope.h`

> **一句定位**：raylib 2D 攝影機模式的 RAII 守衛，建構時 `BeginMode2D`、解構時 `EndMode2D`，確保每幀世界繪製在正確的攝影機 scope 內完成。

## 職責

`CameraScope` 是一個不可複製、不可移動的 RAII 守衛，包住 raylib 的 `BeginMode2D / EndMode2D` 對：建構子接受 `const Camera2D&`，立即呼叫 `::BeginMode2D`（將 `Camera2D` 的引擎型別欄位對映到 raylib 的 `::Camera2D`）；解構子呼叫 `::EndMode2D`。

必須建立於 `DrawScope`（`BeginDrawing / EndDrawing`）的區間內，並在 `DrawScope` 結束前解構。以「內層 `{}` 區塊 + 堆疊解構順序」自然保證此點（先進後出）——在 `EndDrawing` 之後才呼叫 `EndMode2D` 在 raylib 中為未定義行為。

不可複製亦不可移動：防止被移走的 `CameraScope` 在解構時重複呼叫 `EndMode2D`。

在 `View::Draw` 中，`CameraScope` 包住整個「世界物件」繪製層（玩家、NPC、道具等），使這些物件在攝影機座標系（追蹤玩家、縮放後）下繪製，而 HUD / 選單等 UI 元素在 `CameraScope` 解構後再繪製（螢幕座標系，不受攝影機影響）。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::render::CameraScope`**：
  - `CameraScope(const Camera2D& cam) noexcept`：呼叫 `::BeginMode2D`（欄位對映：`offset/target → ::Vector2`，`rotation / zoom` 直接轉）。
  - `~CameraScope()`：呼叫 `::EndMode2D()`。
  - 複製與移動均顯式 `= delete`。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`::BeginMode2D / ::EndMode2D`）、`include/engine/render/Camera2D.h`（攝影機參數型別）。
- **被誰使用（往內）**：`src/ui/View.cpp`（在每幀 Draw 中包住世界繪製層）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/render 層；在 `View` 的每幀 Draw 中，包住「世界層繪製」的 scope，確保物件在攝影機轉換後的座標系下正確渲染。

## OO 概念與設計重點

純 [RAII](../concepts/oo-raii.md) 守衛模式：把必須成對的 raylib 呼叫（`BeginMode2D / EndMode2D`）以建構 / 解構封裝，消除忘記呼叫 `EndMode2D` 的風險。不可複製 + 不可移動確保「每個建構對應唯一一次解構」的不變式。欄位對映（`Camera2D → ::Camera2D`）是 `CameraScope` 建構子的唯一職責，把 raylib 隔離在此一行。與 `DrawScope` 形成嵌套 RAII 層次（DrawScope 在外，CameraScope 在內）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/CameraScope.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/CameraScope.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md) · [DIP：IRenderer](../concepts/arch-dip-renderer.md)
