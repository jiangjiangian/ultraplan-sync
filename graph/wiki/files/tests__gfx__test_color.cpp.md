---
id: file:tests/gfx/test_color.cpp
type: test
path: tests/gfx/test_color.cpp
domain: tests
bucket: gfx
loc: 54
classes: []
sources: ["tests/gfx/test_color.cpp"]
---
# `test_color.cpp`

> **一句定位**：驗證 `Color` 的預設建構（不透明黑色）、聚合初始化、`WithAlpha` 的不可變語意，以及 `Colors::` 調色盤常數的預期值與相等比較運算子。

## 職責

此測試以五個 TEST_CASE 完整覆蓋 `engine/math/Color.h` 的公開合約。`Color` 是 engine 層不依賴 raylib 的顏色資料型別（`{r,g,b,a}` 四個 `uint8_t`），本測試確保它的值語意與不可變 builder 語意正確。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("Color：預設建構為不透明黑色")`：`constexpr Color c;` 斷言 `r==g==b==0`，`a==255`。
- `TEST_CASE("Color：以 {r,g,b,a} 聚合初始化")`：`constexpr Color c{10,20,30,200}`；驗證四個分量。
- `TEST_CASE("Color::WithAlpha 回傳覆寫 alpha 的新 Color，原物件不變")`：以 `constexpr` 驗證不可變語意——`base.WithAlpha(128)` 回傳新物件，`base.a` 仍為 255。
- `TEST_CASE("Colors:: 調色盤含預期的常見顏色")`：`Black.r==0`、`White.r==255`、`White.a==255`、`Blue.b>200`。
- `TEST_CASE("Color 相等運算子逐分量比較")`：兩個相同 RGBA 的物件 `a==b`；alpha 不同時 `a!=c`。

## 相依與在架構中的位置

- **#include（往外）**：`engine/math/Color.h`（唯一依賴）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料型別測試）

## OO 概念與設計重點

`constexpr` 的廣泛使用體現了 `Color` 是真正的字面量型別（literal type），可在編譯期計算。`WithAlpha` 的不可變語意讓算繪路徑能以管線風格（`color.WithAlpha(128)` 傳給 `DrawRect`）而不需要臨時物件管理。相等運算子是讓各種算繪測試中 `HasColor()` 搜尋得以運作的基礎設施。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/gfx/test_color.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/gfx/test_color.cpp) · [← 全檔索引](../files-index.md)
