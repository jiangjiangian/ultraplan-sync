---
id: file:tests/quest/test_ch2_quest.cpp
type: test
path: tests/quest/test_ch2_quest.cpp
domain: tests
bucket: quest
loc: 370
classes: []
sources: ["tests/quest/test_ch2_quest.cpp"]
---
# `test_ch2_quest.cpp`

> **一句定位**：完整規格化 Ch2 主線——「圖書館管理員→喝飲料叫醒學霸→撿筆記→換回」的硬性關卡鏈、借傘與真傘的並存、背包清理，以及延後清關轉場到市集。

## 職責

此測試以十一個 TEST_CASE 規格化 Ch2 的全部邏輯。Ch2 的主線結構是串行硬性關卡：先見管理員（`kFlagMetLibrarian`），才能叫醒學霸（消耗一瓶飲料）；叫醒後才出現筆記、才能換回（三張筆記齊全）；換回後對話關閉才清關轉場。

特別驗證兩個 bug 修正：
1. **借傘與真傘並存**：玩家同時持有管理員借傘（`kFlagLibrarianUmbrella`）和學霸換回的真傘（`HeldUmbrella::True`），背包同時列出兩列，歸還借傘不弄丟真傘。
2. **換回後筆記旗標清除**：`TryRescueBookworm`（換回階段）必須清掉三個筆記旗標，讓背包的任務紙張那一列消失。

## 關鍵內容（類別 / 函式 / 資料）

- `GiveNotes(Player&)`：設定 `kFlagFoundNote1/2/3`，輔助建構「已撿齊筆記」狀態。
- `TEST_CASE("ResolveOpenerSubState：Ch2 librarian 的 (b) 受叫醒旗標守門")`：學霸叫醒前 sub=0，叫醒後 sub=1。
- `TEST_CASE("ResolveOpenerSubState：Ch2 bookworm 由 (a) 沉睡→(c) 已叫醒→(d) 已換回")`：三狀態路由，Ch1 不受影響。
- `TEST_CASE("TryRescueBookworm：叫醒步驟消耗飲料；換回步驟需要筆記")`：章節不符/對象不符為無操作；無飲料只提示；有飲料叫醒消耗；叫醒後無筆記不換回；有筆記換回 +5、給 True 傘、冪等。
- `TEST_CASE("叫醒學霸恰好從背包消耗一瓶提神飲料")`：持有 2 瓶時只扣 1 瓶；第二次對話不扣。
- `TEST_CASE("圖書館管理員借出管理員的傘")`：叫醒前不借；叫醒後借（`HasUmbrella()==true`、`kFlagLibrarianUmbrella`、非真傘）；背包一列；冪等。
- `TEST_CASE("Ch2 借傘與換回的真傘並存，歸還借傘不弄丟真傘")`：叫醒+借傘+換回後，背包同時有真傘列和借傘列；`TryReturnLibrarianUmbrella` +10 且借傘列消失而真傘保留。
- `TEST_CASE("TryRescueBookworm：叫醒前持有筆記永不觸發換回")`：防禦性測試，即使筆記齊全沉睡學霸也不換回。
- `TEST_CASE("LiftChapter2Clear：延後到已換回且對話關閉後才生效")`：三個阻擋條件（未換回、對話開啟、章節不符）與通過條件。
- `TEST_CASE("見過圖書館管理員前學霸無法被叫醒")`：持飲料但未見管理員→導向提示不消耗；`TryMeetLibrarian` 對象/章節不符；見管理員後才可叫醒。
- `TEST_CASE("見過管理員前學霸對話被導向她")`：對話面的導向路由驗證。
- `TEST_CASE("換回學霸筆記會清掉背包的任務紙張那一行")`：換回後三個筆記旗標清除，`kItemNotes` 列消失。
- `TEST_CASE("Ch2 主線經既有主幹抵達幕間市集")`：端到端走完主線，斷言 `Interlude_Market`、`returnTo==Chapter3_SportsDay`。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter2Quest.h`、`ChapterGate.h`、`ItemCatalog.h`，`game/dialog/DialogOpener.h`、`DialogSource.h`、`DialogState.h`，`engine/events/EventBus.h`，`game/entities/Player.h`，`game/state/SemesterStateMachine.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純邏輯測試，不驅動 GameController）

## OO 概念與設計重點

Ch2 的串行硬性關卡（管理員→學霸→筆記→換回）體現了「任務進度是不可跳過的狀態機」的設計原則。`BuildInventoryRows` 作為 View 層的純函式（輸入 Player 狀態，輸出顯示列表），讓背包顯示的正確性可在無 UI 的測試中驗證。借傘/真傘並存的 bug 修正測試展示了「單一 hold 槽 vs. 旗標驅動」的細微語意差異。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch2_quest.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch2_quest.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
