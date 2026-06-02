---
id: file:src/ui/EndingView.cpp
type: source
path: src/ui/EndingView.cpp
domain: ui
bucket: 
loc: 427
classes: []
sources: ["src/ui/EndingView.cpp"]
---
# `EndingView.cpp`

> **一句定位**：四種結局的完整渲染實作——從 `EndingSummary` DTO 繪製結局雨傘、標題、開場字卡、「你為何走到這裡」敘事區塊、結算統計卡片與三選項底部選單。

## 職責

此檔屬於 ui 層，是結局畫面的全部顯示邏輯，完全不碰 `World` / `Player`（MVC 純度）——它只渲染接收到的 `EndingSummary` DTO。

**資料層（匿名命名空間）**：`caption(state)` 各結局開場字卡；`reasonLines(state)` 每個結局 2–3 行劇情台詞（全部已烘焙進字形圖集，無缺字風險）；`pathLabel(state)` 路線標籤；`endingTextColor(state, a)` 結局 B 偏灰、其他白色；`endingUmbrellaLook(state)` 以結局為鍵選擇雨傘外觀（A→TrueBlue、B→CursedPurple、D→FragileBroken、C→UglyGreen）；`conditionsFired(summary)` 只列出本次判定中實際成立的條件（結局 A 永遠三項 AND、B 列出成立的析取項、D 列體諒 + 業力條件、C 列 boughtUgly 或平穩收尾）；`endingMenuLabel(choice)` 三個底部選單標籤的唯一來源。

**`EndingCardStrings()`**：匯出所有結局相關的字串（每個 caption / pathLabel / reasonLines + 強制觸發全部 conditionsFired 分支），供字形掃描測試驗證圖集完整性。

**`DrawEndingCard(...)`**：五個繪製區段：（1）不透明黑背板；（2）頂端置中雨傘字形；（3）標題 + 開場字卡（EAW 感知置中換行）；（4）「你為何走到這裡」敘事區塊（淡色標籤 + reasonLines，`DrawCenteredWrapped` 換行）；（5）結算統計面板（業力、条件清單綠字、路線標籤，面板寬度取最寬列）；（6）底部三選項水平選單（`"> "` 插字符 + 金色高亮、`"  "` 暗色未選），選單正下方導覽提示。

**文字量測**：`CenteredX`、`TextWidthPx`、`CellsForWidth`、`DrawCenteredWrapped` 四個輔助函式以 EAW 字格模型（CJK=2 格，每格約 sz/2 px）替代不存在的 `IRenderer::MeasureText`。

## 關鍵內容（類別 / 函式 / 資料）

- `IsEndingState(SemesterState)` — 判斷是否為四個結局狀態之一（供 `View.cpp` 提前返回）。
- `EndingMenuChoiceAt(int)` — 以模運算把任意游標索引夾到三個選項（防越界）。
- `EndingMenuLabel(EndingMenuChoice)` — 公開標籤取值器（`BackToTitle`/`RestartGame`/`Quit`）。
- `EndingCardStrings()` — 字形掃描測試用字串匯出。
- `DrawEndingCard(IRenderer&, EndingSummary&, string_view title, float alpha, float W, float H, int menuCursor)` — 完整結局卡片渲染。
- `conditionsFired(EndingSummary&)` — 本次判定已成立的條件列表（選擇性顯示析取項）。
- `endingUmbrellaLook(SemesterState)` — 以結局為鍵選擇傘外觀（修正「體諒卻顯示醜傘」bug）。

## 相依與在架構中的位置

- **#include（往外）**：`EndingView.h`、`IRenderer.h`、`Rect.h`/`Vec2.h`/`Color.h`、`UmbrellaGlyph.h`（雨傘字形）、`DialogLayout.h`（`CellWidth`/`WrapToCells`）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `View::RenderEnding` 呼叫 `DrawEndingCard`，由字形掃描測試呼叫 `EndingCardStrings`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：View 層；`View::Draw` 偵測結局狀態後提前返回並呼叫 `RenderEnding → DrawEndingCard`，完全取代世界渲染。

## OO 概念與設計重點

此檔嚴格遵守 [MVC](../concepts/arch-mvc.md) 的 View 只讀 Model 的紅線：`DrawEndingCard` 接受純資料 DTO `EndingSummary`，完全不觸碰 `World` 或 `Player`。`conditionsFired` 的「只顯示成立者」設計讓結算卡片對每個結局都是誠實的：結局 B 可能有 1–3 個不同理由，只顯示那次遊玩中真正觸發的，而非固定模板。`endingUmbrellaLook` 的「以結局為鍵」（而非以旗標組合為鍵）是消除「體諒卻顯示醜傘」bug 的設計決策——前者保證畫面與判定一致。底部選單的插字符 + 金色高亮讓選取在灰階下也明確，是無障礙考量。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/ui/EndingView.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/ui/EndingView.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [DIP Renderer](../concepts/arch-dip-renderer.md)
