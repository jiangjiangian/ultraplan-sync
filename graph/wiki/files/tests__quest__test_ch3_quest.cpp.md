---
id: file:tests/quest/test_ch3_quest.cpp
type: test
path: tests/quest/test_ch3_quest.cpp
domain: tests
bucket: quest
loc: 255
classes: []
sources: ["tests/quest/test_ch3_quest.cpp"]
---
# `test_ch3_quest.cpp`

> **一句定位**：完整規格化 Ch3 物物交換鏈（香腸→大聲公→雨傘情報）、操場跑圈閘門、背包行隨交易乾淨更換、指示燈（`!`）逐環移動，以及清關轉場到幕間市集（returnTo Ch4）。

## 職責

此測試以八個 TEST_CASE 規格化 Ch3 的主要玩法機制。Ch3 的核心是 A→B→C 的物物交換鏈，搭配操場跑圈閘門與逐步揭露的 `!` 指示燈。

主要不變式：
- 鏈的前提是完整的：亂序、章節不符、未跑操場圈均為無操作；各環不重複給獎。
- 操場跑圈（`UpdateSportsLap`）需要玩家真正沿跑道帶走完整一圈；站在中央不算。
- 背包行隨每次交易乾淨切換（交出的立刻消失、下一件出現、情報屬知識不在背包）。
- `Ch3IndicatorVisible` 一次只亮一環：A 在跑圈前就亮（教玩家第一步）→ A 交易後只 B 亮 → B 交易後只 C 亮 → C 揭露後全滅。
- Ch3 清關由 `UmbrellaClaimed{"TrueUmbrella"}` 事件觸發，轉場到市集且 `returnTo==Chapter4`。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("TryAdvanceCh3Trade：物物交換鏈每次對話依序推進一環")`：從亂序無操作、跑圈閘門、三環逐一交易（含 karma 累計：+3、+3、+5）、到整鏈耗盡的完整路徑。
- `TEST_CASE("Ch3 物物交換鏈乾淨地換背包行（香腸 -> 大聲公 -> 無）")`：用 `BuildInventoryRows` 驗證每次交易後的背包內容。
- `TEST_CASE("跑圈前找 A 會在劇情中呈現操場跑圈提示")`：訂閱 ShowMessage 事件，確認發出含「操場」的提示訊息。
- `TEST_CASE("Ch3IndicatorVisible：A->B->C 的「!」一次只揭露一環")`：逐步設旗標並驗證 `Ch3IndicatorVisible` 在每個階段的回傳值；非鏈上的任務給予者（ta）始終亮。
- `TEST_CASE("World::UpdateSportsLap：完整跑完操場一圈會設下 Flag_SportsLapDone")`：以 40 個等角度點模擬圓形路徑，驗證 `SportsLapProgress==1.0f` 且旗標設定；站在中央永遠不累積。
- `TEST_CASE("ResolveOpenerSubState：Ch3 鏈上 NPC 依各自旗標由 (a)->(b)")`：A/B/C 三個 NPC 在各自旗標下的子狀態切換。
- `TEST_CASE("Ch3 清關：領取道具箱的 TrueUmbrella -> 幕間市集且 returnTo 設為 Ch4")`：直接發布 `UmbrellaClaimed{"TrueUmbrella"}` 事件，斷言 `Interlude_Market`、`returnTo==Chapter4_Finals`。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter3Quest.h`、`ItemCatalog.h`，`game/dialog/DialogOpener.h`，`engine/events/EventBus.h`，`game/controller/EventWiring.h`，`game/entities/Player.h`，`game/state/SemesterStateMachine.h`，`game/world/World.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（混合純邏輯與 World 整合測試；`UpdateSportsLap` 需要 World，其餘不需要）

## OO 概念與設計重點

`!` 指示燈「一次只亮一環」的設計讓玩家的注意力被導向下一個目標，避免任務多頭困惑。`A 在跑圈前就亮`（修正過去「第一步無線索」）的行為改變有專門的測試案例釘住，防止反向迴歸。`UpdateSportsLap` 的幾何模擬測試（真實圓形路徑 vs. 中央站立）是端到端功能測試而非純邏輯，因此需要 `World` 物件。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch3_quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch3_quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
