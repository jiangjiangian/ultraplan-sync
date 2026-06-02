---
id: "file:tests/ui/test_chapter_card.cpp"
type: test
path: tests/ui/test_chapter_card.cpp
domain: tests
bucket: ui
loc: 237
classes: [Spy]
sources: ["tests/ui/test_chapter_card.cpp"]
---
# `test_chapter_card.cpp`

> **一句定位**：驗證章節轉場大卡（「傘不見了 / 找到傘了」書擋）的分類邏輯、文案、計時狀態機與繪圖行為。

## 職責

本測試檔針對 `ChapterCardKind`、`ChapterCardState`、`DrawChapterCard` 這三個主要 API 進行單元測試，完全不需要 GL 環境（以 `Spy` IRenderer 攔截）。

測試對象是兩類書擋卡：**Lost 卡**（傘不見了）在每次章節開始時出現；**Found 卡**（找到傘了）在章節清關後轉入市集時出現。走向結局或同狀態轉場不觸發任何卡。

計時狀態機固定了淡入（`kFade` 秒）→ 停留 → 淡出 → 自動消失的四階段生命週期，並驗證 `Dismiss()` 立即清除（玩家可略過）。此外，`reducedMotion=true` 路徑要求 `Alpha()` 從 t=0 即為 1.0（無漸變）。

換行測試以寬 800 / 480 / 360 三種螢幕寬度驗證標題與副標文字列不溢出面板，使用 `dialog::CellWidth` 量測東亞字寬。

## 關鍵內容（類別 / 函式 / 資料）

- `Spy`（local struct）：實作 `IRenderer`，記錄 `rects`、`rectColors`、`texts`、`textX`、`textSize`，用於斷言繪圖原語而無需 GL。
- `Has(Spy, needle)` / `HasRectRGB(Spy, Color)`：輔助搜尋函式。
- `ChapterCardForTransition(from, to)` — 被測：依前後狀態回傳 `ChapterCardKind`（Lost / Found / None）。
- `ChapterCardHeadline(kind, state)` — 被測：第一章用「傘，不見了」；其餘章節用「傘又掉了」；Found 用「找到傘了」。
- `ChapterCardSubtitle(kind, state)` — 被測：Lost 卡標出章節名稱；Found 卡輸出「這一章，過去了」。
- `ChapterCardState` — 被測：`Trigger` 武裝、`Step(dt)` 推進時間、`Alpha(reducedMotion)` 回傳透明度、`Dismiss()` 立即清除。
- `DrawChapterCard(renderer, state, w, h)` — 被測：未啟用時不繪任何東西；Lost / Found 卡畫出對應傘外觀色塊。

## 相依與在架構中的位置

- **#include（往外）**：`ui/ChapterCard.h`（受測主體）、`engine/render/IRenderer.h`（Spy 基底）、`game/state/SemesterState.h`（轉場枚舉）、`game/dialog/DialogLayout.h`（CellWidth 換行斷言）、`game/gfx/UmbrellaGlyph.h`（傘外觀色值）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 實作 `include/engine/render/IRenderer.h`。
- **每幀管線 / MVC 角色**：測試層，不在每幀管線中；對應 View 的 `DrawChapterCard` 函式。

## OO 概念與設計重點

本檔是純 doctest 單元測試，採用 **Spy 模式（Test Double）** 注入 `IRenderer` 的假實作，讓繪圖呼叫可在無 GL 環境驗證。體現了 [DIP + IRenderer](../concepts/arch-dip-renderer.md) 的可測試性收益。`reducedMotion` 路徑固定了無障礙功能不會靜默退化。

## 連結

[🕸 圖譜節點](../../index.html#node=file:tests/ui/test_chapter_card.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_chapter_card.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
