---
id: file:src/game/quest/Chapter1Quest.cpp
type: source
path: src/game/quest/Chapter1Quest.cpp
domain: game
bucket: quest
loc: 163
classes: []
sources: ["src/game/quest/Chapter1Quest.cpp"]
---
# `Chapter1Quest.cpp`

> **一句定位**：第一章（加退選之亂）的任務邏輯——歸還苦主傘、歸還申請書、阿姨醜傘購買，以及 `!` 指示的三階段依序點亮。

## 職責

本檔在 `nccu` 命名空間內實作第一章的所有任務鉤子，以自由函式形式由 `GameController` 在互動階段呼叫。

**`TryReturnVictimUmbrella`**：苦主傘歸還的核心邏輯。三階段：①尚未承諾→空操作（`!kFlagPromisedVictim`）；②承諾但空手→發布「先去找學長」提示；③持有 `kFlagHasVictimUmbrella`→靜默授予真傘（`SetHeldUmbrella(True)` + `SetFlag(kFlagHasTrueUmbrella)`，但「不」發布 `UmbrellaClaimed`），讓 `DialogOpener` 先路由苦主到 (d) 交換回顧，再由 `LiftChapter1Clear` 在對話關閉後才發布 `UmbrellaClaimed` 驅動章節通關。這是防止「交換對話被提前關閉」回歸缺陷的結構性修正。

**`TryReturnTaForm`**：申請書交還助教。持有 `kFlagFoundForm` 且未完成時，給 karma +5、設 `kFlagHelpedTACh1`、清除 `kFlagFoundForm`（背包欄消失）。`kFlagHelpedTACh1` 是跨章節情分旗標，驅動 Ch2–Ch4 的助教漣漪。

**`LiftChapter1Clear`**：每非對話幀輪詢。持有真傘且對話已關閉且尚未清關時，以「ShowMessage 先 → UmbrellaClaimed 後」的正確順序發布，設 `kFlagClearChapter1`（一次性鎖）。順序與 `TrueUmbrella::BeClaimed` 的事件配對一致，使章節通關提示可見於 HUD 前景。

**`TryBuyAuntieUglyUmbrella`**：阿姨醜傘購買（Ch1 唯一）。以 `choiceLabel=="購買醜綠傘"` 精確比對，冪等守衛（已持醜傘返回）。呼叫 `player.DeductMoney(kCh1UglyUmbrellaPrice)` 為把關者，錢不夠則發布 `msg::kInsufficientFunds` 並返回 false。扣款成功後授予 `HeldUmbrella::Ugly` 並以 `vendor::msg` 格式發布花費/餘額提示。刻意「不」設 `kFlagBoughtUglyUmbrella`（那是 Ch4 集英樓攤販的 Ending C 鎖）。

**`Ch1IndicatorVisible`**：三階段 `!` 點亮邏輯：苦主在第 1 步（未承諾）和第 3 步（學長完成後）亮起；學長只在第 2 步（已承諾但尚未確認學長選項）亮起；其餘 NPC 維持名冊 `isQuestGiver` 位元。

## 關鍵內容（類別 / 函式 / 資料）

- `TryReturnVictimUmbrella(EventBus&, Player&, npcId, state)`：三階段傘歸還，靜默授予後等對話閉幕。
- `TryReturnTaForm(Player&, npcId, state)`：申請書交還，設 `kFlagHelpedTACh1`。
- `LiftChapter1Clear(EventBus&, Player&, state, DialogState&)`：對話後輪詢通關，ShowMessage→UmbrellaClaimed。
- `TryBuyAuntieUglyUmbrella(EventBus&, Player&, npcId, choiceLabel, state)` → `bool`：阿姨醜傘購買，DeductMoney 把關。
- `Ch1IndicatorVisible(npcId, isQuestGiver, Player&)` → `bool`：三階段 `!` 點亮。
- `kCh1UglyUmbrellaPrice`：醜傘售價常數（定義於 `Chapter1Quest.h`）。
- `vendor::msg::kInsufficientFunds` / `kPurchasedPrefix` / 等：花費/餘額文案共用常數。

## 相依與在架構中的位置

- **#include（往外）**：`Chapter1Quest.h`、`EventBus.h`（Publish）、`Player.h`（旗標/業力/持傘/金錢）、`DialogState.h`（`dialog.Active()` 閘控）、`ItemCatalog.h`（`ItemInfoFor` 取中文名稱）、`VendorMessages.h`（花費/餘額文案）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 的互動鉤子直接呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：每幀管線的「E 互動 `RunInteractHooks`」與「結局判定」階段均呼叫本檔函式（`LiftChapter1Clear` 在非對話幀每幀輪詢）。

## OO 概念與設計重點

本檔體現了複雜任務邏輯的「鉤子函式」設計：每個函式只負責一個任務節點，並以嚴格的狀態前置條件（旗標）確保冪等性。`LiftChapter1Clear` 的事件發布順序是顯式的正確性保證，說明行內的設計決策文字本身就是一種「可執行文件」。[Observer](../concepts/pat-observer.md) 體現於 `UmbrellaClaimed` 觸發章節通關——`Chapter1Quest.cpp` 不直接操作 `SemesterStateMachine`，而是透過事件驅動。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/quest/Chapter1Quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/quest/Chapter1Quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
