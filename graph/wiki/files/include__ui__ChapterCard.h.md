---
id: file:include/ui/ChapterCard.h
type: header
path: include/ui/ChapterCard.h
domain: ui
bucket: 
loc: 145
classes: [ChapterCardState]
sources: ["include/ui/ChapterCard.h"]
---
# `ChapterCard.h`

> **一句定位**：章節邊界「書檔大字卡」（「傘又掉了」／「找到傘了」）的 View 側計時狀態機、渲染函式與字形覆蓋率列舉，完全由 FSM 轉場驅動、不新增任何事件。

## 職責

本檔定義了章節轉場時覆蓋全幕的大字卡系統：字卡出現在玩家進入新章節（Lost — 「傘又掉了」）或剛完成某章（Found — 「找到傘了」）時，第一章以「傘，不見了」作為開場變體，第四章因改由 `EndingView` 收尾而不觸發 Found 字卡。

關鍵設計是字卡**完全在 View 側驅動**：`View` 每幀比對 `world.Semester().Current()` 與它自己記住的前一狀態（`lastSemester_`），偵測到 FSM 邊界即呼叫 `ChapterCardForTransition()` 判斷要不要觸發、觸發哪一種。不發布任何 `EventType`，不修改 World，是純 View 裝飾。由 `Time::DeltaSeconds()`（自動跑流程下固定 1/60）推進時鐘，故腳本化重播每次都在相同幀渲染字卡，存檔逐位元不變。

`ChapterCardState` 持有目前字卡種類、標題、副標題和經過秒數。`Alpha()` 在 `kFade=0.3s` 內漸升到 1、持留、最後 `kFade` 秒漸降，開啟 `reducedMotion` 時全程不透明直到 `kTotal=2.2s` 硬切。

`ChapterCardStrings()` 列舉所有可能渲染的字面字串，供字型 glyph-scan 測試確保沒有字形缺漏。

## 關鍵內容（類別 / 函式 / 資料）

- `ChapterCardKind`（`enum class`）：`None`、`Lost`（章節開始）、`Found`（章節完成）。
- `ChapterCardForTransition(from, to) -> ChapterCardKind`（`[[nodiscard]]`）：純分類函式；`to` 為 Chapter* 時 Lost，`from` 為 Chapter1/2/3 且 `to` 為 Interlude 時 Found，其他（結局轉場）為 None。
- `ChapterCardHeadline(kind, to) -> std::string_view`（`[[nodiscard]]`）：Lost 第一章返回「傘，不見了」、其他章節返回「傘又掉了」；Found 恆返回「找到傘了」；None 返回空字串。
- `ChapterCardSubtitle(kind, to) -> std::string`（`[[nodiscard]]`）：Lost 返回章節名（由目的狀態組出）；Found 返回短結語。
- `ChapterCardStrings() -> std::vector<std::string>`（`[[nodiscard]]`）：列舉所有標題／副標題字串，供 glyph-scan 測試使用。
- `ChapterCardState`（class）：
  - 常數：`kFade=0.3f`（淡入淡出窗）、`kTotal=2.2f`（總顯示秒數）。
  - `Active()` / `Kind()` / `Headline()` / `Subtitle()` / `Elapsed()` — 狀態讀取。
  - `Alpha(reducedMotion=false) -> float` — 計算 [0,1] 不透明度。
  - `Trigger(kind, headline, subtitle)` — 武裝新字卡（種類 None 則清除）。
  - `Step(dt)` — 推進時鐘；超過 `kTotal` 自動清除（`->None`）。
  - `Dismiss()` — 按鍵略過，立即清除。
  - 私有：`kind_`、`headline_`、`subtitle_`、`elapsed_`。
- `DrawChapterCard(r, card, screenW, screenH, reducedMotion=false)`：繪製全幕暗底＋標題＋副標題＋大型雨傘字形（Lost → 破傘骨架、Found → 藍色傘面），整張以 `Alpha()` 淡入淡出。

## 相依與在架構中的位置

- **#include（往外）**：`game/state/SemesterState.h`（轉場分類依賴 `SemesterState` 列舉）；`<string>`、`<string_view>`、`<vector>`。
- **被誰使用（往內）**：`include/ui/View.h`（View 持有 `ChapterCardState chapterCard_` 成員）；`src/ui/ChapterCard.cpp`（`ChapterCardForTransition` 等自由函式的實作）；`src/ui/View.cpp`（`UpdateChapterCardTransition` 與 `RenderOverlays`）；`tests/ui/test_chapter_card.cpp`、`tests/ui/test_font_ui_glyph_scan.cpp`（單元測試與字形覆蓋測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：純 View 層（ui domain）。`ChapterCardState` 是 View 的渲染側狀態，其生命週期完全在 View 內，不進 World 的物件容器，不影響模型或控制器。

## OO 概念與設計重點

字卡以 **View 側狀態機** 驅動（而非模型事件）是核心設計選擇，文件中明確說明理由：「不新增任何 EventType、不發布、不變更模型…故自動跑流程的存檔逐位元不變」。`ChapterCardStrings()` 的 **字形覆蓋率測試支援** 是一個可測性設計範本：只要有新字串被新增到渲染路徑而未加入列舉，CI 字型測試就會失敗。`Alpha()` 的 `reducedMotion` 參數使同一函式能服務一般玩家與無障礙玩家，是 **策略注入** 的輕量應用，對應 [pat-strategy](../concepts/pat-strategy.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/ui/ChapterCard.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/ui/ChapterCard.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Strategy](../concepts/pat-strategy.md)
