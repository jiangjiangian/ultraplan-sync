---
id: file:include/ui/MessageView.h
type: header
path: include/ui/MessageView.h
domain: ui
bucket: 
loc: 42
classes: []
sources: ["include/ui/MessageView.h"]
---
# `MessageView.h`

> **一句定位**：短暫 HUD 提示框（toast）的純渲染函式 `DrawHudMessage`，支援雙通道（Top/Bottom）、CJK 折行與「減少動畫」無障礙淡出，不持有狀態。

## 職責

`DrawHudMessage` 是 HUD 提示橫幅的唯一渲染入口。它是其輸入的純函式：每次呼叫完全由傳入的 `message`、`age`、`reducedMotion` 與 `slot` 決定輸出，不持有任何內部狀態（與 `DrawDialog`、`DrawEndingCard` 同）。

兩個提前返回條件：`message` 為空，或 `age >= kHudTtl`（已過期）——此時什麼都不畫。這與 `World::HudExpired()` 的判定條件一致，確保「模型認為過期」與「View 不渲染」兩者同步。

視覺效果：半透明背景 + CJK 折行文字，在最後 `kHudFade` 秒內整條橫幅淡至透明。`reducedMotion` 開啟時把淡出收斂為硬切（底層呼叫 `HudToastFadeT(remaining, kHudFade, reduced)` 的 `reduced=true` 路徑直接回傳 1.0）。

`slot` 決定垂直位置：`HudSlot::Bottom`（預設）位於螢幕底部上方約 28 px；`HudSlot::Top` 以固定間距浮在 Bottom 之上，使兩通道同幀並存時皆清晰可讀。slot 只影響視覺位移，兩通道的淡出、TTL、折行邏輯完全共用。

`kHudTtl` 與 `kHudFade` 定義於 `game/world/HudTiming.h`（已 include），讓 World 不必引入本 ui 標頭即可使用同一份計時常數（見 `HudTiming.h` 的設計說明）。

## 關鍵內容（類別 / 函式 / 資料）

- `DrawHudMessage(r, message, age, screenW, screenH, reducedMotion=false, slot=HudSlot::Bottom)`：
  - `r`：`IRenderer&`，所有繪製經此注入，不直接呼叫 raylib，可無頭 spy 測試。
  - `message`：提示文字；為空或過期時空操作。
  - `age`：從 `World::HudAge(slot)` 取得的計齡秒數。
  - `reducedMotion`：開啟時淡出硬切。
  - `slot`：`HudSlot::Bottom`（一般訊息）或 `HudSlot::Top`（章節／結局重大提示）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/events/HudSlot.h`（`HudSlot` 列舉，`slot` 參數型別）；`game/world/HudTiming.h`（`kHudTtl`、`kHudFade` 計時常數）；`<string>`。
- **被誰使用（往內）**：`src/ui/MessageView.cpp`（`DrawHudMessage` 的實作）；`src/ui/View.cpp`（`RenderOverlays` 中為 Top 與 Bottom 兩通道各呼叫一次）；`tests/dialog/test_dialog_skip.cpp`、`tests/ui/test_hud_reset.cpp`、`tests/ui/test_message_view.cpp`、`tests/ui/test_two_hud_channels.cpp`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層（ui domain），在 `RenderOverlays` 階段為 Top 與 Bottom 兩個通道各呼叫一次，以 `const World&` 讀取 `HudMessage(slot)` 與 `HudAge(slot)`。

## OO 概念與設計重點

`DrawHudMessage` 是無狀態純渲染函式的典型範本，與 `DrawDialog`、`DrawEndingCard` 形成一組設計一致的 ui 函式群。`reducedMotion` 參數讓同一函式支援兩種無障礙行為，而非分成兩個函式或在函式內硬判環境變數，是 [pat-strategy](../concepts/pat-strategy.md) 的精簡應用。透過 `IRenderer` 的渲染注入讓提示框可在無頭測試中以 spy 替身驗證，對應 [arch-dip-renderer](../concepts/arch-dip-renderer.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/MessageView.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/MessageView.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
