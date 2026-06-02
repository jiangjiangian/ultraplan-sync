---
id: file:include/engine/render/DrawScope.h
type: header
path: include/engine/render/DrawScope.h
domain: engine
bucket: render
loc: 34
classes: [DrawScope]
sources: ["include/engine/render/DrawScope.h"]
---
# `DrawScope.h`

> **一句定位**：每幀繪圖區間的 RAII 守衛，建構時 `BeginDrawing`、解構時 `EndDrawing`，確保幀繪圖對每幀都成對完成且不遺漏。

## 職責

`DrawScope` 是一個不可複製、不可移動的 RAII 守衛，對 raylib 的 `::BeginDrawing / ::EndDrawing` 進行封裝：建構子呼叫 `::BeginDrawing()`，解構子呼叫 `::EndDrawing()`。

在 `SceneManager::Run` 的每幀迭代中，`DrawScope` 實例被建立為局部變數，確保無論場景 Update 還是 Draw 中是否拋出例外，`EndDrawing` 都會被呼叫（雖然實際上在無例外的遊戲迴圈中，這點不如 GC 語言重要，但仍是良好實踐）。

不可複製亦不可移動：防止被複製或移走的守衛重複呼叫 `EndDrawing`。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::render::DrawScope`**：
  - `DrawScope() noexcept`：呼叫 `::BeginDrawing()`。
  - `~DrawScope()`：呼叫 `::EndDrawing()`。
  - 複製與移動均顯式 `= delete`。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`::BeginDrawing / ::EndDrawing`）——raylib 隔離在引擎層。
- **被誰使用（往內）**：`src/app/SceneManager.cpp`（主迴圈每幀建立 DrawScope）、`src/app/scenes/GameplayScene.cpp`（可能在場景層直接使用）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/render 層；是每幀管線的最外層 scope，`IScene::Draw` 在 `DrawScope` 的 `BeginDrawing / EndDrawing` 之間被呼叫。

## OO 概念與設計重點

最簡單的 [RAII](../concepts/oo-raii.md) 守衛模式：兩行程式碼（建構呼 `BeginDrawing`，解構呼 `EndDrawing`），消除忘記配對的風險。`DrawScope` 和 `CameraScope` 形成嵌套 RAII 層次，是 raylib 的「BeginDrawing > BeginMode2D > ... > EndMode2D > EndDrawing」呼叫堆疊的 C++ 資源守衛映射。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/DrawScope.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/DrawScope.h) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md)
