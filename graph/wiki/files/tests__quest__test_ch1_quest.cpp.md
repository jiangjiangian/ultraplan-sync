---
id: file:tests/quest/test_ch1_quest.cpp
type: test
path: tests/quest/test_ch1_quest.cpp
domain: tests
bucket: quest
loc: 393
classes: [Capture]
sources: ["tests/quest/test_ch1_quest.cpp"]
---
# `test_ch1_quest.cpp`

> **一句定位**：完整規格化 Ch1 互惠主線——苦主授予的雙重前提、交換對話先播的延後清關、詛咒傘的 Ending B 路徑、苦主之傘的延後生成、阿姨醜傘購買邏輯，以及助教申請書的跨章情分。

## 職責

此測試是 Ch1 關卡設計的核心規格文件，以十個 TEST_CASE 覆蓋整個 Ch1 主線與支線的邊界條件。使用 `Capture` 輔助結構透過 RAII `ScopedSubscribe` 捕捉 `UmbrellaClaimed` 和 `ShowMessage` 事件，讓測試在不依賴輸入的情況下驗證事件驅動的狀態機轉換。

主要驗證的不變式：
- `TryReturnVictimUmbrella` 需要「承諾 + 持有苦主傘」兩個前提才觸發，且授予是**靜默的**（UmbrellaClaimed 被壓住，等待 (d) 交換對話先播）。
- `LiftChapter1Clear` 在對話**關閉後**才發布 UmbrellaClaimed → 轉場到市集，且 `returnTo == Chapter2`。
- 苦主之傘是**延後生成**（`MaybeSpawnChapter1VictimUmbrella`），只在 `kFlagSuitSeniorChoiceMade` 後才出現一次，離開章節隨名冊清除。
- 阿姨醜傘購買扣 80 元、給 `HeldUmbrella::Ugly`，但不設 `kFlagBoughtUglyUmbrella`（Ending C 的鎖由 Ch4 Vendor 負責）。
- 助教申請書情分（`HelpedTA_Ch1`）在 Ch3 才兌現 +5，而且確實只兌現一次。

## 關鍵內容（類別 / 函式 / 資料）

- `Capture`：持有兩個 `EventBus::Subscription` RAII 物件，捕捉 `umbrellaClaims` 向量和 `lastMessage`；以 `[[nodiscard]] MakeCapture()` 建立。
- `MakePlayer()`：建立 `Vec2{0,0}` 的全新 Player。
- `CountQuestPickups(World&)`：計算 World 中 `QuestFlagPickup` 的數量（用於驗證延後生成前後的數量變化：1→2）。
- `TEST_CASE("TryReturnVictimUmbrella：唯有承諾並歸還後才授予真傘")`：五種前提狀態的逐一驗證，含冪等性。
- `TEST_CASE("TryReturnVictimUmbrella：未承諾時即使持傘也永不授予")`：防禦性測試。
- `TEST_CASE("苦主交換對話先於 Ch1 清關播放（延後清關）")`：授予→對話開啟→清關延後→對話關閉→清關觸發→轉場 Ch2；一次性守門不重複。
- `TEST_CASE("Ch1 道德傘仍可領取（保留 Ending B 路徑）")`：`CursedUmbrella::BeClaimed` 的完整效果（旗標+污染+UmbrellaClaimed）及冪等性。
- `TEST_CASE("Ch1 苦主之傘撿取物會設下 Flag_HasVictimUmbrella")`：驗證 `ChapterQuestItems(Ch1)` 確實只有一個條目。
- `TEST_CASE("見到苦主前，西裝學長只純台詞帶過、不開選單")`：承諾前/後的對話路由差異。
- `TEST_CASE("苦主之傘延後到 Flag_SuitSeniorChoiceMade 後才生成，離開章節隨名冊清除")`：`MaybeSpawnChapter1VictimUmbrella` 的生命週期。
- `TEST_CASE("MaybeSpawnChapter1VictimUmbrella 在非 Ch1 時為無操作")`。
- `TEST_CASE("Ch1 阿姨醜傘購買扣 80 元並授予手持 Ugly，不設 Ending C 旗標")`：含情境不符、資金不足的無操作驗證。
- `TEST_CASE("Ch1 阿姨醜傘購買有資金守門")`。
- `TEST_CASE("Ch1 交還申請書設下 HelpedTA 情分，並在 Ch3 兌現 +5")`：跨章漣漪的完整路徑。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter1Quest.h`、`Chapter3Quest.h`、`ChapterGate.h`、`ChapterQuestItems.h`，`game/dialog/DialogState.h`、`DialogOpener.h`、`DialogSource.h`，`engine/events/EventBus.h`，`game/controller/EventWiring.h`，`game/entities/Player.h`、`QuestFlagPickup.h`、`CursedUmbrella.h`、`TransparentUmbrella.h`，`game/state/SemesterStateMachine.h`，`game/world/World.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純邏輯測試，不驅動 GameController）

## OO 概念與設計重點

`Capture` 使用 [RAII Subscription](../concepts/oo-raii.md) 模式：訂閱在 `Capture` 物件離開作用域時自動取消，防止測試間的訂閱洩漏。[Observer 模式](../concepts/pat-observer.md)（EventBus）讓各種跨模組的副作用可透過事件捕捉而不直接查詢狀態。延後清關的設計體現了「狀態機轉換必須在 UI 確認（對話關閉）後才能執行」的架構紅線。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch1_quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch1_quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [RAII](../concepts/oo-raii.md) · [State](../concepts/pat-state.md)
