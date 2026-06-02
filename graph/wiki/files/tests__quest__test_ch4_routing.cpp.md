---
id: file:tests/quest/test_ch4_routing.cpp
type: test
path: tests/quest/test_ch4_routing.cpp
domain: tests
bucket: quest
loc: 101
classes: []
sources: ["tests/quest/test_ch4_routing.cpp"]
---
# `test_ch4_routing.cpp`

> **一句定位**：驗證 `ResolveOpenerSubState` 對 Ch4 各 NPC 的開場子狀態路由（依旗標與 karma 分支），並確認路由本身不得誤設旗標或誤改 karma。

## 職責

此測試檔針對 `nccu::ResolveOpenerSubState`（定義於 `Chapter2Quest.h` / 通用對話路由層）在 `Chapter4_Finals` 語境下的行為。重點是「路由只是讀取狀態、不應產生副作用」這個不變量。

四個 TEST_CASE 分別覆蓋：
1. **西裝學長**：無 `kFlagHelpedSenior` → sub-state 0（退化）；有旗標且 karma>70 → 1（(b) 感謝路徑）；有旗標且 karma<30 → 2（(c) 尷尬路徑）；中間值 karma → 0（(a) 中性）。
2. **助教**：`kFlagHasProfessorTrap` 時 → 2（(c) 怒意）；再加 `kFlagHelpedTACh1` 時 (b) 優先（idx 1 勝 idx 2）；`bookworm` 的 (a) 未救 → 2、救回 → 1；`victim` 的傘已在手 → 0（釋懷），承諾旗標但無傘 → 1（淡漠），有傘 override → 0。
3. **Ch4 助教 (c) 路由不得誤設 `kFlagHelpedTACh1`**：路由到 Ch4 (c) 不得設 Ch1 範圍限定的旗標也不得改 karma。
4. **Ch1 助教獎勵重述仍會生效（守門完好）**：以 `Chapter1_AddDrop` 確認自動套用確有作用（karma 改變或旗標被授予）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ResolveOpenerSubState：Ch4 西裝學長依 HelpedSenior 與 karma 分支")`：三分支 + 退化案例，使用三個獨立 `Player` 物件測試不同 karma 區段。
- `TEST_CASE("ResolveOpenerSubState：Ch4 助教 (b)/(c) 優先序，以及 bookworm/victim 路由")`：複合優先序驗證。
- `TEST_CASE("OpenNpcDialog：Ch4 助教 (c) 路由不得誤設 HelpedTA")`：副作用守門——呼叫 `OpenNpcDialog` 後比較 flag 與 karma 是否未被修改。
- `TEST_CASE("OpenNpcDialog：Ch1 助教獎勵重述仍會生效（守門完好）")`：反向確認 Ch1 範圍確實有作用（正向不變量）。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`Chapter2Quest.h`（`ResolveOpenerSubState`）、`DialogOpener.h`（`OpenNpcDialog`）、`DialogSource.h`（`SetContentDir`）、`DialogState.h`、`Player.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試對話開場子狀態路由，這是 E 互動後 `OpenNpcDialog` 調用前的決策點。

## OO 概念與設計重點

「路由不帶副作用」是此測試的核心契約，對應 SOLID 的 SRP（解析子狀態 vs. 套用 karma 是兩個不同職責）。TEST_CASE 4 用一個不對稱的 `CHECK((p.GetKarma() != k0 || p.HasFlag(...)))` 來驗證不變量存在而非特定值，展示了「斷言行為而非實作細節」的良好測試風格。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch4_routing.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch4_routing.cpp) · [← 全檔索引](../files-index.md)
