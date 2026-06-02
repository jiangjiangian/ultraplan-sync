---
id: "file:tests/ui/test_rain_hud_redundant.cpp"
type: test
path: tests/ui/test_rain_hud_redundant.cpp
domain: tests
bucket: ui
loc: 81
classes: []
sources: ["tests/ui/test_rain_hud_redundant.cpp"]
---
# `test_rain_hud_redundant.cpp`

> **一句定位**：驗證雨量 HUD 的色盲冗餘設計——`RainTierPrefix()` 依三個門檻（<60 / [60,85) / ≥85）回傳固定 2 字寬的等級前綴，使等級不靠色相也能辨識。

## 職責

雨量 HUD 原本只用白→金→紅三色傳達等級，但紅綠色盲玩家難以區分金色與紅色。新設計在文字前加上 2 字寬前綴作為冗餘通道：

- 雨量 < 60：`"  "`（兩個空格，保留欄位對齊）
- 60 ≤ 雨量 < 85：`" !"`（警告）
- 雨量 ≥ 85：`"!!"`（危急）

本測試驗證：
1. 三個門檻的分界點（0.0f、30.0f、59.99f → `"  "`；60.0f、72.5f、84.99f → `" !"`；85.0f、90.0f、100.0f → `"!!"`）。
2. **不論等級，前綴長度恆為 2**（`size() == 2u`），確保雨量行欄位對齊不受等級影響。
3. **端到端組字驗證**：複製 `View::Draw` 的 `snprintf("%.*s rain: %d%%", ...)` 格式，驗證最終字串含有正確的前綴與雨量數字（`" ! rain: 70%"`、`"!! rain: 90%"`）。

## 關鍵內容（類別 / 函式 / 資料）

- `RainTierPrefix(float rain)` — 被測純函式，回傳 `std::string_view`，長度固定為 2。
- 門檻常數：60.0f（calm→warn）、85.0f（warn→crit）。
- `snprintf` 格式字串：`"%.*s rain: %d%%"`，對應 View.cpp 的 HUD 組字邏輯。

## 相依與在架構中的位置

- **#include（往外）**：`ui/RainHud.h`（受測主體）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層輔助函式測試；`RainTierPrefix` 在每幀 Draw 的 HUD 組字中呼叫。

## OO 概念與設計重點

純 doctest 單元測試，測試對象是一個純函式（無狀態、無副作用）。「固定 2 字寬」的不變量直接反映了設計決策（等寬欄位對齊），測試把這個設計決策變成可執行的規格。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_rain_hud_redundant.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_rain_hud_redundant.cpp) · [← 全檔索引](../files-index.md)
