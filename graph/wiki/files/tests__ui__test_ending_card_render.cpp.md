---
id: "file:tests/ui/test_ending_card_render.cpp"
type: test
path: tests/ui/test_ending_card_render.cpp
domain: tests
bucket: ui
loc: 302
classes: [Spy]
sources: ["tests/ui/test_ending_card_render.cpp"]
---
# `test_ending_card_render.cpp`

> **一句定位**：驗證四結局結算卡的繪圖正確性——傘色塊由結局而非原始旗標決定、文案與判定條件依實際資料條件化顯示，以及文字列不溢出畫面文字區。

## 職責

本測試檔以 `Spy`（IRenderer 假實作）攔截 `DrawEndingCard` 的繪圖呼叫，驗證結局結算卡的渲染契約。測試對象是 `EndingSummary` 結構體攜帶的業力值、結局旗標與 `DrawEndingCard` 函式。

**傘色塊正確性**是本測試的核心：修正先前「體諒助教卻顯示醜傘」的 bug——卡片必須依 `state` 欄位決定傘形圖（A→藍、B→暗紫、C→醜傘綠、D→破傘），而非依 `consoledTA`、`boughtUgly` 等原始旗標。

**條件化文案**：結局 B 依實際生效的條件（`tookCursed`、`karma<0`、`finaleChoiceMade && !consoledTA`）選擇性呈現，而非全部顯示。

**換行閘**：對三種螢幕寬度（800/520/420）驗證每列文字右緣不超出畫面文字區，使用 `dialog::CellWidth` 量測。

此外也固定「恰好一個結算標題錨點」不變量，以及 `IsEndingState()` 對所有四個結局枚舉均回傳 true。

## 關鍵內容（類別 / 函式 / 資料）

- `Spy`（local struct）：實作 `IRenderer`，擷取 `rects`、`rectColors`、`texts`、`textX`、`textSize`，及計數 `sprites`。
- `Has(Spy, needle)` / `CountWith(Spy, needle)` / `HasRectRGB(Spy, Color)`：三個輔助搜尋函式。
- `IsEndingState(SemesterState)` — 被測：回傳 Ending_A/B/C/D 為 true，其餘為 false。
- `DrawEndingCard(renderer, EndingSummary, label, alpha, screenW, screenH)` — 被測：依 state 路由傘色塊、條件化文案、業力數字、路線標籤。
- `EndingSummary` — 受測資料結構：含 `state`、`karma`、`hasTrueUmbrella`、`consoledTA`、`tookCursed`、`finaleChoiceMade`、`boughtUgly` 等欄位。

## 相依與在架構中的位置

- **#include（往外）**：`ui/EndingView.h`（受測主體）、`engine/render/IRenderer.h`（Spy 基底）、`game/state/SemesterState.h`、`game/dialog/DialogLayout.h`、`game/gfx/UmbrellaGlyph.h`（傘外觀色值）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：`Spy` 實作 `include/engine/render/IRenderer.h`。
- **每幀管線 / MVC 角色**：測試層，對應 View 的 `DrawEndingCard` 函式；結局資料屬於 Model。

## OO 概念與設計重點

以 **Spy 模式** 注入 [IRenderer](../concepts/arch-dip-renderer.md)，在無 GL 環境驗證多型繪圖路徑。結局 D 的補充測試尤其證明 D 的完整渲染路徑沒有借用其他結局的分支——體現了「開閉原則」在結局系統的落實。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/ui/test_ending_card_render.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/ui/test_ending_card_render.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[DIP Renderer](../concepts/arch-dip-renderer.md)
