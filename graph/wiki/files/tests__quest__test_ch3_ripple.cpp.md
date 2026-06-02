---
id: file:tests/quest/test_ch3_ripple.cpp
type: test
path: tests/quest/test_ch3_ripple.cpp
domain: tests
bucket: quest
loc: 84
classes: []
sources: ["tests/quest/test_ch3_ripple.cpp"]
---
# `test_ch3_ripple.cpp`

> **一句定位**：驗證 Ch3 漣漪——四個 NPC 的開場依 Ch1/Ch2 旗標路由，以及 `ProfessorTrap` 的 -10 每章 Ch3 只結算一次且與 Ch2 的扣分鍵彼此獨立。

## 職責

此測試以兩個 TEST_CASE 規格化 Ch3 的跨章漣漪系統。Ch3 的 NPC 開場台詞路由比前兩章更複雜，因為需要反映整個學期的累積狀態：

- `bookworm`：預設為未救回狀態 (b)，Ch2 救回後才切到 (a)。
- `ta`：預設 (a)，有 `HelpedTA_Ch1` 時為 (c)（在 Ch3 運動會兌現情分）。
- `victim`：預設未承諾 (b)，承諾後 (a)。
- `suit_senior`：預設 (a)，有 `HelpedSenior` 時為 (b)（物物交換鏈提示）。

`TryApplyCh3Ripple` 的 `ProfessorTrap` -10 與 Ch2 的 `kFlagCh2RippledTA` 鍵**彼此獨立**：即使 Ch2 已扣過 -10，Ch3 仍獨立再扣 -10，兩章不共用同一個一次性鍵。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ResolveOpenerSubState：Ch3 漣漪依 Ch1/Ch2 旗標路由")`：四個 NPC（bookworm/ta/victim/suit_senior）各自的路由表驗證，以及 Ch2 bookworm 路由不受 Ch3 邏輯影響的反向驗證。
- `TEST_CASE("TryApplyCh3Ripple：ProfessorTrap 每章 Ch3 -10 一次，且鍵彼此獨立")`：無旗標/章節不符→無操作；`ProfessorTrap` + Ch3 → -10 並設 `kFlagCh3RippledProfTrap`；重複呼叫不加倍；Ch2 鍵（`kFlagCh2RippledTA`）不妨礙 Ch3 再扣 -10（獨立計算）。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter3Quest.h`，`game/dialog/DialogOpener.h`，`engine/events/EventBus.h`，`game/entities/Player.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純邏輯測試）

## OO 概念與設計重點

「各章獨立一次性鍵」的設計確保每章的 karma 結算不受其他章節影響，讓多章的業力效應可以明確疊加而不互抵。`bookworm` 在 Ch3 預設顯示「未救回」狀態的設計意圖：若玩家跳過 Ch2 救援步驟，Ch3 能反映這個事實，讓世界狀態有連貫性。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch3_ripple.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch3_ripple.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
