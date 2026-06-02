---
id: file:tests/quest/test_ripple_seed_flags.cpp
type: test
path: tests/quest/test_ripple_seed_flags.cpp
domain: tests
bucket: quest
loc: 114
classes: []
sources: ["tests/quest/test_ripple_seed_flags.cpp"]
---
# `test_ripple_seed_flags.cpp`

> **一句定位**：驗證 Ch1 各「壞傘」領取時種下正確的漣漪旗標（ProfTrap/Cursed），含冪等性（`isActive_` 守門防重發）；好傘和脆傘不種任何漣漪旗標。

## 職責

此測試檔鎖定「傘領取 → 種旗標」這個漣漪系統的最前端行為，測試對象是四種傘的 `BeClaimed` 方法。

四個正向 TEST_CASE + 一個負向：
1. **ProfessorTrapUmbrella**：`BeClaimed` 後設 `kFlagHasProfessorTrap`，`HasUmbrella()==true`；第二次呼叫冪等（旗標維持，無重設）。
2. **CursedUmbrella**：`BeClaimed` 後設 `kFlagTookCursedUmbrella`，`GetCursedTaint()==1`，karma 在撿取當下不變（代價移到後續各章污染衰減）；第二次呼叫冪等，污染值不重複提升。
3. **TrueUmbrella 冪等性**：發布 `UmbrellaClaimed` 只一次（訂閱計數 `claimed` 不超過 1）；第二次 `BeClaimed` 是無操作，防止 Ch1/Ch3 事件接線重複轉場。
4. **FragileUmbrella 冪等性**：同樣只發 `UmbrellaClaimed` 一次。
5. **好傘/脆傘不種漣漪旗標**：`BeClaimed` 後 `kFlagHasProfessorTrap` 和 `kFlagTookCursedUmbrella` 均為 false。

## 關鍵內容（類別 / 函式 / 資料）

- `ProfessorTrapUmbrella::BeClaimed(&Player)`：種 `kFlagHasProfessorTrap`。
- `CursedUmbrella::BeClaimed(&Player)`：種 `kFlagTookCursedUmbrella`，增加 `GetCursedTaint`。
- `TrueUmbrella::BeClaimed(&Player)`：發布 `UmbrellaClaimed("TrueUmbrella")`，標記 `isActive_=false`。
- `FragileUmbrella::BeClaimed(&Player)`：發布 `UmbrellaClaimed("FragileUmbrella")`，標記 `isActive_=false`。
- `TEST_CASE("領取 ProfessorTrapUmbrella 種下 Flag_HasProfessorTrap 一次")`。
- `TEST_CASE("領取 CursedUmbrella 種下 Flag_TookCursedUmbrella 並提升污染值")`：污染值設計的變更說明（撿取不再直接扣 karma）。
- `TEST_CASE("TrueUmbrella::BeClaimed 具冪等性（不會重複發 UmbrellaClaimed）")`：透過計數器 + `ScopedSubscribe` 驗證。
- `TEST_CASE("FragileUmbrella::BeClaimed 具冪等性...")`。
- `TEST_CASE("好傘／脆傘不會種下任何漣漪旗標")`。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`CursedUmbrella.h`、`ProfessorTrapUmbrella.h`、`TrueUmbrella.h`、`FragileUmbrella.h`、`Player.h`、`EventBus.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層的 `BeClaimed` 方法，對應每幀管線中 `Collision`/`Pickup` 階段觸發的傘領取事件。

## OO 概念與設計重點

`isActive_` 守門的冪等性驗證是「縱深防禦」原則的測試面：呼叫端雖然有活動過濾，但 `BeClaimed` 自身也必須安全地被多次呼叫。`CursedUmbrella` 把 karma 代價從即時扣減改為污染值（各章衰減）是一個值得注意的設計演進，此測試精確釘住「撿取當下 karma 不變」的新契約。[Observer 模式](../concepts/pat-observer.md)（`UmbrellaClaimed` 事件）的冪等測試直接對應 Ch1/Ch3 章節接線的安全性。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ripple_seed_flags.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ripple_seed_flags.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [Template Method](../concepts/pat-template.md)
