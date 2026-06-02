---
id: "file:include/engine/render/TextBuilder.h"
type: header
path: include/engine/render/TextBuilder.h
domain: engine
bucket: render
loc: 91
classes: [TextBuilder, nccu, nccu, nccu]
sources: ["include/engine/render/TextBuilder.h"]
---
# `TextBuilder.h`

> **一句定位**：文字繪製建構器——以流暢介面組裝「文字內容＋位置＋字級＋顏色」，並在 CJK 字型已載入時走 `DrawTextEx` 正確顯示中文，否則退回 raylib 預設字型。

## 職責

`TextBuilder` 是 engine render 層的 header-only 文字繪製工具。以建構器（Builder）模式提供 `At()`、`Size()`、`Color()` 三個流暢設定器，最後以 `Draw()` 繪製或 `Measure()` 量測版面尺寸。

CJK 字型感知是此類別的核心設計關切：`Draw()` 在呼叫時以 `IsCJKFontLoaded()` 做執行期分支——字型已載入則以 `::DrawTextEx(CJKFont(), …)` 繪製（正確渲染中文）；否則退回 `::DrawText`（ASCII fallback）。`Measure()` 也以相同邏輯分支，確保「繪製寬度」與「量測寬度」取自同一路徑，背板可精準貼合文字。

字距公式 `size / 10.0f` 與 raylib `::DrawText` 的內部慣例一致，且同時套用在 `Draw()` 和 `Measure()` 兩處，維持量測與繪製的吻合。

`TextBuilder` 不做分頁或斷行（那是 `DialogLayout` 的職責）；它只負責把一個字串正確地畫出來或量出大小。廣泛被 UI 元件（HUD、選單、對話框、說明頁）和各 Scene（標題、載入、選角）使用，是整個 render 層使用頻率最高的工具之一。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `class TextBuilder` | Header-only 建構器；持有 `text_`、`position_`（預設 {0,0}）、`size_`（預設 10）、`color_`（預設黑）。 |
| `explicit TextBuilder(std::string text)` | 以文字內容建構；移動語意。 |
| `At(Vec2 p)` | 設定左上角座標；回傳 `*this`（流暢介面）。 |
| `Size(int s)` | 設定字級；回傳 `*this`。 |
| `Color(Color c)` | 設定文字顏色；回傳 `*this`。 |
| `Draw()` | 繪製：CJK 字型存在時走 `::DrawTextEx`，否則 `::DrawText`。 |
| `Measure()` | 量測並回傳 `Vec2`（寬、高像素）；與 `Draw()` 同路徑保持一致。 |
| `GetPosition()` / `GetSize()` / `GetColor()` | 讀取目前設定（測試用途）。 |

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（直接使用 raylib 文字 API）、`include/engine/math/Color.h`、`include/engine/render/Font.h`（提供 `IsCJKFontLoaded()`/`CJKFont()`）、`include/engine/math/Vec2.h`。
- **被誰使用（往內）**：`include/engine/render/RaylibRenderer.h`（`DrawText` 方法委派給此）、`src/app/scenes/CharacterSelectScene.cpp`/`LoadingScene.cpp`/`TitleScene.cpp`（各場景直接繪製文字）、`src/ui/` 下多個 UI 元件（`HelpPageView`、`ObjectiveBar`、`StatusPanel`、`MenuAffordance`、`PauseMenu`、`View.cpp`）、`tests/gfx/test_text_builder.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（MVC 的 V）繪圖工具；不參與模擬管線，在每幀繪製階段被各 UI 元件呼叫。

## OO 概念與設計重點

`TextBuilder` 是典型的**Builder 模式**應用：把多個可選配置（位置、字級、顏色）收進流暢介面，讓呼叫端以一行鏈式語句完成設定與繪製，避免每次都要傳四個參數給 free function。

CJK 感知的執行期分支是**Strategy 模式**的隱性體現：同一 `Draw()` 入口依字型狀態選擇不同的 raylib 路徑，而呼叫端完全感知不到此分支的存在。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/TextBuilder.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/TextBuilder.h) · [← 全檔索引](../files-index.md)
