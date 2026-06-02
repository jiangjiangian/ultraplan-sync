---
id: file:tests/gfx/test_text_builder.cpp
type: test
path: tests/gfx/test_text_builder.cpp
domain: tests
bucket: gfx
loc: 37
classes: []
sources: ["tests/gfx/test_text_builder.cpp"]
---
# `test_text_builder.cpp`

> **一句定位**：驗證 `TextBuilder` 的流暢式設定器（`At` / `Size` / `Color`）能正確保存狀態，預設值為位置 (0,0)、字級 10、顏色黑，且串接時各設定器回傳 `*this`。

## 職責

此測試以三個 TEST_CASE 覆蓋 `TextBuilder` 的 builder 介面合約。`TextBuilder` 是 engine 層用於組裝文字繪製命令的輕量型別，讓 View 層可以鏈式設定位置、字級、顏色後傳給 `IRenderer::DrawText`。

測試驗證：
1. 設定器呼叫後 `GetPosition()` / `GetSize()` / `GetColor()` 均能正確反映。
2. 預設建構時 size=10、color=Black。
3. `At().Size().Color()` 的串接回傳同一物件的參考。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("TextBuilder 流暢式設定器設定後狀態應被保存")`：對 `TextBuilder{"hi"}` 呼叫 `At({10,20}).Size(16).Color(Colors::Red)` 後斷言三個 getter。
- `TEST_CASE("TextBuilder 預設值：位置 (0,0)、字級 10、顏色黑")`：新建構的 `TextBuilder{"x"}` 驗證 size==10、color==Colors::Black。
- `TEST_CASE("TextBuilder 串接時各設定器回傳自身參考")`：斷言 `&ref == &t`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/render/TextBuilder.h`（受測），`engine/math/Color.h`，`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 builder 邏輯測試）

## OO 概念與設計重點

與 `Camera2D` 測試相同的流暢介面串接正確性驗證模式。這類測試雖然短小，但防止了「設定器回傳值型別而非參考」的靜默錯誤——在鏈式呼叫時不會報編譯錯，但設定結果會丟失。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_text_builder.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_text_builder.cpp) · [← 全檔索引](../files-index.md)
