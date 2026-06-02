---
id: file:include/ui/EndingView.h
type: header
path: include/ui/EndingView.h
domain: ui
bucket: 
loc: 79
classes: [EndingSummary]
sources: ["include/ui/EndingView.h"]
---
# `EndingView.h`

> **一句定位**：四種結局全螢幕畫面的渲染 DTO（`EndingSummary`）、字形覆蓋率列舉與 `DrawEndingCard` 函式聲明，純 View 層、不碰 World 邏輯。

## 職責

本標頭定義了結局畫面渲染所需的全部公開介面：

`EndingSummary` 是一個純渲染 DTO，由 `View.cpp` 從 `World`／`Player` 萃取六個基本值（`state`、`karma`、五個旗標），傳給 `DrawEndingCard`。`DrawEndingCard` 本身「不碰 World／Player」，只讀取此 DTO，維持 MVC 純度——View 層從模型讀取狀態，而不查詢它。

注解詳細說明了五個旗標的含義與對應的結局路線：
- `hasTrueUmbrella`（結局 A：奪回真傘）
- `consoledTA`（結局 A 需：終局體諒助教）
- `tookCursed`（結局 B：詛咒傘路線）
- `boughtUgly`（結局 C：買下醜綠傘）
- `finaleChoiceMade`（終局助教結算已發生）

結局 B 的「最後質問助教」（`coldFinale`）由 `DrawEndingCard` 以 `finaleChoiceMade && !consoledTA` 推導，DTO 只攜帶原始旗標而非最終判定結果，保持 DTO 的資訊密度最小化。

`EndingCardStrings()` 列舉所有可能出現在結局畫面的字面字串（四種結局的標題、理由句、條件標籤），供字型 glyph-scan 測試確保沒有字形缺漏。

`DrawEndingCard` 也從 `EndingMenuModel.h` 轉出 `EndingMenuChoice`、`EndingMenuChoiceAt`、`EndingMenuLabel`，使只 include 本標頭的呼叫端也能解析結局選單相關名稱。

## 關鍵內容（類別 / 函式 / 資料）

- `EndingCardStrings() -> std::vector<std::string>`（`[[nodiscard]]`）：列舉所有結局畫面字面字串，供 glyph-scan 測試走訪；純資料，不碰 raylib。
- `EndingSummary`（struct）：結局畫面純渲染 DTO：
  - `state`（`SemesterState`，預設 `Ending_C`）：當前結局狀態。
  - `karma`（`int`）：最終業力值。
  - `hasTrueUmbrella`、`consoledTA`、`tookCursed`、`boughtUgly`、`finaleChoiceMade`（`bool`）：五個結局判定旗標。
- `DrawEndingCard(r, summary, title, alpha, screenW, screenH, menuCursor=0)`：繪製全螢幕結局畫面——黑底＋標題＋開場字卡＋理由文案＋業力結算卡＋底部三選項選單（0 回首頁、1 重新開始、2 結束）；`menuCursor` 指定高亮項；以 `alpha` 淡入。

## 相依與在架構中的位置

- **#include（往外）**：`game/state/EndingMenuModel.h`（`IsEndingState`、選單相關函式）；`game/state/SemesterState.h`（`SemesterState` 列舉）；`<string>`、`<string_view>`、`<vector>`。
- **被誰使用（往內）**：`src/ui/EndingView.cpp`（`DrawEndingCard` 的實作）；`src/ui/View.cpp`（`RenderEnding` 組出 `EndingSummary` 並呼叫 `DrawEndingCard`）；`tests/state/test_ending_gate.cpp`、`tests/ui/test_ending_card_render.cpp`、`tests/ui/test_ending_menu.cpp`、`tests/ui/test_font_ui_glyph_scan.cpp`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 View 層（ui domain）。當 FSM 處於結局狀態時，`View::Draw` 提前分支到 `RenderEnding`，用 `DrawEndingCard` 完全取代正常世界渲染。

## OO 概念與設計重點

`EndingSummary` 是標準的 **Data Transfer Object（DTO）** 模式：薄薄一層只攜帶渲染所需的最小資訊，讓 `DrawEndingCard` 成為純輸入的渲染函式，可由 spy 替身測試（與 `DrawDialog`、`DrawHudMessage` 同）。`EndingCardStrings()` 的字形覆蓋率支援與 `ChapterCardStrings()` 是同一套設計——把「所有可能被渲染的字串」集中列舉，由 CI 測試在任何字形缺漏時自動失敗，比執行期出現缺字方塊早數個數量級發現問題。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/EndingView.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/EndingView.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md)
