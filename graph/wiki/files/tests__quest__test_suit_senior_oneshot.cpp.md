---
id: file:tests/quest/test_suit_senior_oneshot.cpp
type: test
path: tests/quest/test_suit_senior_oneshot.cpp
domain: tests
bucket: quest
loc: 93
classes: []
sources: ["tests/quest/test_suit_senior_oneshot.cpp"]
---
# `test_suit_senior_oneshot.cpp`

> **一句定位**：驗證西裝學長的分支選單具「一次性」語義：`Flag_SuitSeniorChoiceMade` 設下後再對話只剩純台詞重述，防止疊加互斥漣漪旗標；且此守門只作用於學長，不外溢到其他 NPC。

## 職責

此測試檔針對 Ch1 西裝學長對話的 one-shot 機制進行三個面向的驗證：

1. **首次對話有選單**：前置 `kFlagPromisedVictim`（硬性關卡：對苦主承諾後才開啟學長選單），呼叫 `OpenNpcDialog` 後 `DriveToChoiceOrClose` 返回 true，選項非空。

2. **定案後只剩台詞**：設 `kFlagSuitSeniorChoiceMade` 後對話開啟，但 `DriveToChoiceOrClose` 返回 false，選項為空，最終 `d.Active()` 為 false（播完後關閉），且 karma 不變（不重複套用）。

3. **守門不外溢**：設 `kFlagSuitSeniorChoiceMade` 後對 `"victim"` 開啟對話，`DriveToChoiceOrClose` 仍返回 true（苦主仍有 A/B 選項）。

匿名 namespace 的 `DriveToChoiceOrClose` helper 最多推進 64 步，若停在選擇選單回傳 true，以純台詞收尾回傳 false。

## 關鍵內容（類別 / 函式 / 資料）

- `DriveToChoiceOrClose(nccu::DialogState&)`：推進至選單或結束，返回是否停在選單。
- `TEST_CASE("首次與西裝學長對話會出現分支選單")`：前置 `kFlagPromisedVictim`，確認選單出現。
- `TEST_CASE("選擇定案旗標設下後，西裝學長只剩純台詞重述")`：確認台詞重述路徑 + karma 不變。
- `TEST_CASE("守門旗標只作用於西裝學長 — 苦主仍會出現分支")`：負向隔離測試。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`DialogOpener.h`（`OpenNpcDialog`）、`DialogState.h`（`Advance/AtChoice/Choices`）、`DialogSource.h`（`SetContentDir`）、`Player.h`、`SemesterState.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 E 互動路徑上的對話開場邏輯，對應 `GameController` 在按下 E 後呼叫 `OpenNpcDialog` 的行為。

## OO 概念與設計重點

One-shot 選單模式是防止「互斥旗標疊加」的安全守門：`kFlagSuitSeniorChoiceMade` 相當於一個「決策點已消費」的計數旗標。「守門不外溢」測試確保此旗標的語義嚴格限定於一個 NPC，體現了 SRP（每個守門條件只守一個對話節點）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_suit_senior_oneshot.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_suit_senior_oneshot.cpp) · [← 全檔索引](../files-index.md)
