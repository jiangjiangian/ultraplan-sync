---
id: file:src/ui/ChapterCard.cpp
type: source
path: src/ui/ChapterCard.cpp
domain: ui
bucket: 
loc: 225
classes: []
sources: ["src/ui/ChapterCard.cpp"]
---
# `ChapterCard.cpp`

> **一句定位**：章節起訖大字卡的邏輯與渲染——狀態機轉移時觸發「傘，不見了」/「找到傘了」全螢幕節拍，並處理淡入淡出動畫與 EAW 感知的置中換行。

## 職責

此檔屬於 ui 層，實作章節切換時出現的大字卡（`ChapterCardState` + `DrawChapterCard`）以及相關的查詢函式。

**`ChapterCardForTransition(from, to)`**：判斷兩個狀態間的轉移應觸發哪種字卡：進入某章節（遊戲開始或插曲段後返回）→ `Lost`（「傘，不見了 / 傘又掉了」）；章節結束進入市集 → `Found`（「找到傘了」）；其他（結局轉移等）→ `None`。

**`ChapterCardHeadline` / `ChapterCardSubtitle`**：第一章用「傘，不見了」（初次事件），第二至四章用「傘又掉了」（反覆節拍）；各章附上章節副標（「第X章 加退選 / 期中考 / 運動會 / 期末」）。`ChapterCardStrings()` 匯出所有可能的字串供字形掃描測試。

**`ChapterCardState`**：持有 `kind_`、`headline_`、`subtitle_`、`elapsed_`。`Trigger` 武裝字卡、`Step(dt)` 推進計時器（超過 `kTotal` 自動 `Dismiss`）、`Alpha(reducedMotion)` 計算透明度（淡入段 = `elapsed/kFade`、維持 = 1、淡出段 = `(kTotal-elapsed)/kFade`；`reducedMotion=true` 時硬切為 1）。

**`DrawChapterCard`**：以四個繪製步驟渲染字卡：（1）全螢幕半透明變暗層（alpha×170）；（2）橫幅面板（高 150px，上下各一條金線）；（3）標題左側的雨傘字形（`Found` → `TrueBlue`；`Lost` → `FragileBroken`）；（4）標題（40pt，Found 用暖金、Lost 用冷米白）+ 副標（18pt），皆以 EAW 感知的 `DrawCenteredWrapped`（`WrapToCells` + `CenteredX`）置中換行。

## 關鍵內容（類別 / 函式 / 資料）

- `IsChapter(SemesterState)` — 匿名命名空間輔助，判斷是否為四章之一。
- `CenteredX(string_view, int sz, float screenW)` — EAW 字格模型水平置中計算（CJK=2 格、ASCII=1 格，每格約 sz/2 px）。
- `CellsForWidth(float, int)` — 計算指定像素寬度內可容的 EAW 字格數。
- `DrawCenteredWrapped(...)` — 用 `WrapToCells` 換行後逐列置中繪製。
- `ChapterCardForTransition(SemesterState, SemesterState)` — 轉移類型推導。
- `ChapterCardHeadline(ChapterCardKind, SemesterState)` — 標題字串選擇。
- `ChapterCardSubtitle(ChapterCardKind, SemesterState)` — 副標字串選擇。
- `ChapterCardStrings()` — 匯出全部可能字串（供字形掃描測試）。
- `ChapterCardState::Alpha(bool)` / `Trigger(...)` / `Step(float)` / `Dismiss()` — 字卡生命週期方法。
- `DrawChapterCard(IRenderer&, ChapterCardState&, float, float, bool)` — 完整渲染一張字卡。
- `kFade` / `kTotal` — 淡入 / 總時長常數（隱含於 `ChapterCardState`）。

## 相依與在架構中的位置

- **#include（往外）**：`ChapterCard.h`（型別宣告）、`IRenderer.h`（渲染介面）、`Rect.h`/`Vec2.h`/`Color.h`、`UmbrellaGlyph.h`（雨傘字形）、`DialogLayout.h`（`CellWidth`/`WrapToCells`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View.cpp` 的 `UpdateChapterCardTransition` 和 `RenderOverlays` 呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；字卡是純 View 狀態（不進入存檔），由 `View::Draw` 的末端繪製，覆蓋世界 / HUD / 對話 / 選單之上。

## OO 概念與設計重點

EAW 感知的 `CenteredX` / `DrawCenteredWrapped` 函式設計與 `EndingView.cpp`、`MessageView.cpp` 完全一致，是本專案「沒有 IRenderer::MeasureText → 用字格模型替代」的統一方案。`reducedMotion=true` 時 `Alpha` 硬切為 1（完全不透明）展示了無障礙設計直接整合進渲染邏輯而非另外判斷。字卡的 `Step / Dismiss` 生命週期由 View 的渲染時鐘驅動，與遊戲存檔完全解耦。[DIP Renderer](../concepts/arch-dip-renderer.md) 讓渲染呼叫只走 `IRenderer`，字卡邏輯可做 headless 測試。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/ChapterCard.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/ChapterCard.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
