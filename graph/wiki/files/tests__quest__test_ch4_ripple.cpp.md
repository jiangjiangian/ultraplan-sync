---
id: file:tests/quest/test_ch4_ripple.cpp
type: test
path: tests/quest/test_ch4_ripple.cpp
domain: tests
bucket: quest
loc: 128
classes: []
sources: ["tests/quest/test_ch4_ripple.cpp"]
---
# `test_ch4_ripple.cpp`

> **一句定位**：驗證 `TryApplyCh4Ripple` 對各 NPC（學長、學霸、助教、阿姨）的 karma 漣漪：條件觸發、冪等性、獨立性，以及開場子狀態路由函式 `ResolveOpenerSubState` 的咖啡旗標分支。

## 職責

此測試檔覆蓋 `nccu::TryApplyCh4Ripple` 與 `nccu::ResolveOpenerSubState` 兩個 `Chapter4Quest.h` 的核心函式，驗證每個 Ch4 NPC 在前章旗標存在時的 karma 調整行為。

學長（`suit_senior`）的 +10 需同時滿足 `kFlagHelpedSenior` 且 karma>70；學霸（`bookworm`）的 +5 只需 `kFlagBookwormRecovered`；助教（`ta`）的 +10（`kFlagHelpedTACh1`）與 -15（`kFlagHasProfessorTrap`）彼此獨立可疊加（淨 -5 的複合 SUBCASE），各自有結算鍵消耗保證只一次；阿姨（`shop_auntie`）的 +3 需 `kFlagBoughtCoffeeForAuntie`，並以 `kFlagCh4RippledAuntie` 守門。

每個 NPC 的漣漪都有「章節不符或對象不符 → 無操作」的守門測試，確保結算函式不會跨章誤觸。`ResolveOpenerSubState` 的 `shop_auntie` 分支進一步驗證 Ch4 對話開場子狀態由同一個咖啡旗標決定路由（有咖啡 → sub-state 0 直接情報；無咖啡 → sub-state 3 間接情報）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("TryApplyCh4Ripple：學長 +10 只在 (b) 路徑發生，且只一次")`：驗證無旗標不觸發、有旗標但 karma 不足不觸發、有旗標且 karma>70 觸發一次。
- `TEST_CASE("TryApplyCh4Ripple：學霸於 BookwormRecovered 時 +5，且只一次")`：簡單旗標守門 + 冪等。
- `TEST_CASE("TryApplyCh4Ripple：助教 +10 / -15 彼此獨立")` + SUBCASE：獨立觸發與複合淨 -5 的驗證；兩個結算鍵在同一次呼叫後全部消耗。
- `TEST_CASE("TryApplyCh4Ripple：福利社阿姨於咖啡旗標時 +3，且只一次")`：章節不符守門 + 咖啡旗標 + `kFlagCh4RippledAuntie` 冪等守門。
- `TEST_CASE("ResolveOpenerSubState：Ch4 shop_auntie 依咖啡旗標路由")`：無咖啡→3（間接），有咖啡→0（直接）。
- `TEST_CASE("TryApplyCh4Ripple：章節不符／對象不符 -> 無操作")`：跨章守門的負向案例。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`Chapter4Quest.h`（`TryApplyCh4Ripple`、`ResolveOpenerSubState`）、`DialogOpener.h`（`ResolveOpenerSubState` 轉發）、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 E 互動後 Model 層的漣漪計算，對應每幀管線的 `RunInteractHooks` 呼叫點。

## OO 概念與設計重點

本測試集中體現「漣漪系統的冪等性不變量」：每個旗標只結算一次的守門確保玩家反覆接近同一個 NPC 不會累積 karma。複合測試（SUBCASE 兩者皆有）用來驗證 +10 和 -15 兩個結算路徑的相互獨立性，這反映了 ISP（介面隔離）精神：每個漣漪條件獨立評估。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_ripple.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_ripple.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
