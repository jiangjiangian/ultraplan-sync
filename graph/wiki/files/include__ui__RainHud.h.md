---
id: file:include/ui/RainHud.h
type: header
path: include/ui/RainHud.h
domain: ui
bucket: 
loc: 30
classes: []
sources: ["include/ui/RainHud.h"]
---
# `RainHud.h`

> **一句定位**：雨量 HUD 的色弱備援前綴函式 `RainTierPrefix`——依雨量計數值回傳「  」／「 !」／「!!」純文字前綴，使壓力等級不只靠顏色傳達。

## 職責

`RainTierPrefix` 是雨量 HUD 顏色斜坡（白→金→紅）對色盲玩家的純文字補充備援。顏色斜坡對紅綠色弱（deuteran/protan）玩家難以分辨，因此額外加上文字前綴，確保「風險上升」這個訊號不只靠顏色承載（無障礙設計原則：資訊不應僅以顏色傳達）。

三段分界清晰：
- `rm < 60`：`"  "`（平靜，兩個空格維持對齊）
- `60 ≤ rm < 85`：`" !"`（警告，一個嘆號）
- `rm ≥ 85`：`"!!"`（危急，兩個嘆號）

View 在 `StatusPanel` 中把前綴接在 `"rain: NN%"` 前面，使文字通道永不單靠顏色承載資訊。

函式是 `constexpr`，回傳 `std::string_view` 指向靜態字面值，整個程式生命週期持有皆安全；不配置記憶體、不呼叫 raylib。

## 關鍵內容（類別 / 函式 / 資料）

- `RainTierPrefix(float rm) -> std::string_view`（`constexpr noexcept`）：依雨量計數值分三段回傳固定前綴字串（靜態字面值）；`rm >= 85` 時 `"!!"`、`rm >= 60` 時 `" !"`、其他 `"  "`。

## 相依與在架構中的位置

- **#include（往外）**：`<string_view>`（回傳型別）；無其他依賴。
- **被誰使用（往內）**：`src/ui/View.cpp`（`StatusPanel` 中組出雨量讀數字串時前置）；`src/ui/hud/StatusPanel.cpp`（狀態面板的雨量行）；`tests/ui/test_rain_hud_redundant.cpp`（單元測試，驗證三段邊界的前綴值）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層工具函式，在 `RenderHud` 階段的 `DrawStatusPanel` 中每幀呼叫，純計算無副作用。

## OO 概念與設計重點

`RainTierPrefix` 體現了 **無障礙設計（Accessibility）** 的工程實踐：在視覺顏色之外增加文字通道，確保訊號不依賴單一感知模態。`constexpr` + `std::string_view` 回傳靜態字面值是 **零成本抽象** 的範本：呼叫端得到字串，但無任何動態配置。三段閾值（60、85）的選擇與 `RainVignette.h` 的兩段（`rm >= 60` 微弱、`rm >= 85` 較強）一致，確保文字前綴與視覺暗角出現在同一壓力等級（一致的 UX 提示設計）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/RainHud.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/RainHud.h) · [← 全檔索引](../files-index.md)
