---
id: file:include/game/world/HudTiming.h
type: header
path: include/game/world/HudTiming.h
domain: game
bucket: world
loc: 26
classes: []
sources: ["include/game/world/HudTiming.h"]
---
# `HudTiming.h`

> **一句定位**：HUD 訊息的存留秒數（`kHudTtl`）與淡出長度（`kHudFade`）兩個編譯期常數的共用定義點，放在 game 層使 World 不必引入 ui 標頭即可讀取。

## 職責

本標頭只定義兩個 `inline constexpr float` 常數，但其「放在哪裡」是精心設計的架構決策。

`World` 需要在 `HudExpired()` 中以 `kHudTtl` 對 HUD 訊息槽計齡（`TickHud`），以及在 `DismissHud()` 中把計齡值設到 `kHudTtl` 觸發提前過期。若將這兩個常數定義在 `ui/MessageView.h` 中，`World` 就必須引入 ui 標頭，形成 **game → ui** 的反向相依，違反架構分層紅線（`engine` 不反向依賴的同理規則在此處擴展到 game 不主動拉入 ui）。

解法是把常數下放到 `game/world/HudTiming.h`，讓 `World.h` 引入它（game 層引入 game 層，合法），而 `ui/MessageView.h` 也引入同一個標頭（ui 層讀取 game 層常數，符合 app → game / ui → engine 的單向相依）。兩者使用同一份定義，不會漂移。

`kHudFade = 1.0f` 是 `kHudTtl = 4.0f` 最後一秒的淡出窗，`MessageView` 的 `DrawHudMessage` 在「減少動畫」開啟時將此淡出收斂為硬切。

## 關鍵內容（類別 / 函式 / 資料）

- `kHudTtl`（`inline constexpr float = 4.0f`）：HUD 橫幅在畫面上的存留秒數，`World::TickHud` 計齡、`HudExpired` 以此為過期門檻、`DismissHud` 直接設至此值。
- `kHudFade`（`inline constexpr float = 1.0f`）：尾端淡出長度（`kHudTtl` 的最後 `kHudFade` 秒）；`DrawHudMessage` 在「減少動畫」模式下塌縮為硬切。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（無 include）。
- **被誰使用（往內）**：`include/game/world/World.h`（`TickHud`、`HudExpired`、`DismissHud` 都依賴這兩個常數）；`include/ui/MessageView.h`（`DrawHudMessage` 的淡出計算依賴 `kHudFade`）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純資料常數，無管線行為。其值驅動 World 的 `TickHud`（Model 層計齡）與 `DrawHudMessage`（View 層淡出），是跨 MVC 邊界的唯一共享常數。

## OO 概念與設計重點

本檔的設計核心是 **打破循環依賴**：把常數移至 game 層而非 ui 層，明確切斷 `World`（Model）對 ui 標頭的潛在依賴。這是一種精準的 **介面隔離（ISP）** 與 **依賴方向管控** 的應用——把「需要被多方共用的最小資訊單元」提取到雙方都可合法引入的最低公分母位置。類似手法廣泛應用於此專案（如 `GameHelpPages.h` 的頁數常數也因同樣理由放在 game 層）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/HudTiming.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/HudTiming.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
