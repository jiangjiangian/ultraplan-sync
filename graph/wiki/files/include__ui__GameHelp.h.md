---
id: file:include/ui/GameHelp.h
type: header
path: include/ui/GameHelp.h
domain: ui
bucket: 
loc: 151
classes: []
sources: ["include/ui/GameHelp.h"]
---
# `GameHelp.h`

> **一句定位**：「遊戲說明」分頁文字的單一真實來源——兩頁 constexpr 陣列（操作/目標、傘外觀/道具/結局）供標題畫面與遊戲內暫停選單共用，並以 static_assert 確保分頁與扁平清單同步。

## 職責

本標頭是遊戲所有說明文字的**唯一定義點**，供兩處渲染端共用：`TitleScene.cpp` 的標題說明頁，以及 `HelpOverlay.cpp` 的遊戲內暫停說明疊層。兩者若各自維護一份文字，任何措辭修改都必須同步改兩處；集中定義後只需改一個地方。

內容分為兩頁：
- **第 1 頁（`kGameHelpPage1`，13 行）**：操作鍵（WASD/E/Tab/M）與目標（找傘、雨量機制、按 M 暫停凍結雨壓力計）。
- **第 2 頁（`kGameHelpPage2`，18 行）**：雨傘外觀辨識（藍色真傘／破傘骨架／暗紫詛咒傘／螢光綠醜傘）、道具須知（金幣跨章節、消耗品章節限定、撐傘自動減雨）、四種結局（真相大白／屠龍者終成惡龍／風雨同行／破財消災）。

`kGameHelpPages` 是兩個 `std::span<const std::string_view>` 的陣列，提供零複製的分頁視圖；渲染端改讀分頁版本，而非舊版把所有文字擠在單一面板。

回溯相容的扁平清單 `kGameHelpLines` 保留為字形覆蓋率測試與選單提示測試的真實來源，使相關測試仍能逐行走訪所有字串。`kGameHelpClosing` 單獨定義最後一行「破財消災：到集英樓買下醜綠傘。」，讓 glyph-scan 也能涵蓋這一行。

三個 `static_assert` 釘住分頁與扁平清單的一致性：頁數與 `kGameHelpPageCount`（定義於 `game/state/GameHelpPages.h`，故 game 層場景不必拉入此 ui 標頭）同步；`page1.size() + page2.size() == kGameHelpLines.size() + 1`（差 1 行是結語移到獨立常數）；`kGameHelpPage2.back() == kGameHelpClosing`（結語必須是第 2 頁末行）。

## 關鍵內容（類別 / 函式 / 資料）

- `kGameHelpPage1`（`inline constexpr std::array<std::string_view, 13>`）：第 1 頁內容（操作 + 目標，含 2 個空行分隔）。
- `kGameHelpPage2`（`inline constexpr std::array<std::string_view, 18>`）：第 2 頁內容（傘外觀 + 道具須知 + 四種結局）。
- `kGameHelpPages`（`inline constexpr std::array<std::span<const std::string_view>, 2>`）：分頁視圖，渲染端以 `kGameHelpPages[page]` 取當頁行列表。
- `kGameHelpLines`（`inline constexpr std::array<std::string_view, page1.size()+page2.size()-1>`）：扁平行清單（結語除外），供 glyph-scan 與選單測試走訪。
- `kGameHelpClosing`（`inline constexpr std::string_view`）：「破財消災：到集英樓買下醜綠傘。」單行結語，獨立以供 glyph-scan 涵蓋。
- 三個 `static_assert`：確保頁數與 `kGameHelpPageCount` 同步、分頁等於扁平清單 + 結語、第 2 頁末行等於 `kGameHelpClosing`。

## 相依與在架構中的位置

- **#include（往外）**：`game/state/GameHelpPages.h`（`kGameHelpPageCount` 常數，供 `static_assert` 對齊頁數）；標準庫 `<array>`、`<span>`、`<string_view>`。
- **被誰使用（往內）**：`src/app/scenes/TitleScene.cpp`（標題畫面說明頁渲染）；`src/ui/HelpPageView.cpp`（說明頁面繪製 helper）；`src/ui/View.cpp`（遊戲內說明疊層）；`src/ui/overlay/HelpOverlay.cpp`（說明疊層實作）；`tests/ui/test_font_ui_glyph_scan.cpp`（字形覆蓋率測試）；`tests/ui/test_menu_help.cpp`（選單說明測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純資料常數，屬 ui 層。被兩處渲染端（標題場景與遊戲內疊層）讀取，不涉及模型或控制器。

## OO 概念與設計重點

本標頭核心價值是 **單一真實來源（Single Source of Truth）** 加上 **編譯期一致性保證**：三個 `static_assert` 把文字同步的維護工作從「記得手動同步」改為「編譯失敗即提醒」。`std::span` 的零複製分頁視圖是 C++20 的現代化應用，讓分頁與扁平清單共用同一份底層字串儲存而無需複製。`kGameHelpPageCount` 放在 `game/state/GameHelpPages.h`（而非此 ui 標頭）讓 game 層場景（TitleScene、PauseScreen）不必引入 ui 標頭即可讀取頁數，是同 `HudTiming.h` 精神的架構層邊界管控。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/GameHelp.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/GameHelp.h) · [← 全檔索引](../files-index.md)
