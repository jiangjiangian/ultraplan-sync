---
id: file:tests/quest/test_ch2_ripple.cpp
type: test
path: tests/quest/test_ch2_ripple.cpp
domain: tests
bucket: quest
loc: 131
classes: []
sources: ["tests/quest/test_ch2_ripple.cpp"]
---
# `test_ch2_ripple.cpp`

> **一句定位**：驗證 Ch2 漣漪——西裝學長/助教的開場依 Ch1 旗標路由、karma 漣漪每章只結算一次，以及開場自動套用對已路由的漣漪加 0 分（不重複計分）。

## 職責

此測試規格化 Ch2 的「漣漪」系統：Ch1 的選擇在 Ch2 透過 NPC 開場台詞路由（子狀態選擇）和一次性 karma 結算（`TryApplyCh2Ripple`）體現。四個 TEST_CASE 覆蓋：

1. **西裝學長開場路由**：`HelpedSenior`→(b)、`ScoldedSenior`→(c)、預設→(a)。
2. **助教開場路由**：`HelpedTA`→(b)；`ProfessorTrap` 同時存在時→(c) 優先；Ch1 路由不受影響。
3. **TryApplyCh2Ripple 結算**：`HelpedSenior` +3 一次；`ScoldedSenior` karma 中性（Ch1 指正屬理性發言，Ch2 不扣回）但設一次性鍵；`ProfessorTrap` -10 一次；`HelpedTA` 純資訊不計 karma；無旗標/章節不符/對象不符均無操作。
4. **不重複計分**：`OpenNpcDialog` 路由到 (b) 時加 0 karma（已持 `HelpedSenior` 旗標），之後只有 `TryApplyCh2Ripple` 結算一次 +3。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("ResolveOpenerSubState：Ch2 西裝學長依 Ch1 漣漪旗標路由")`：三種旗標組合的子狀態映射。
- `TEST_CASE("ResolveOpenerSubState：Ch2 助教 — ProfessorTrap 優先於 HelpedTA")`：優先序規則，含 Ch1 路由不受影響的反向驗證。
- `TEST_CASE("TryApplyCh2Ripple：每章 Ch2 恰好結算一次 ±3 / -10")`：五個 SUBCASE，覆蓋全部旗標組合的 karma 結算、一次性鍵設定與冪等性。
- `TEST_CASE("不重複計分：開場自動套用對已路由的漣漪加 0")`：`OpenNpcDialog` 後 karma 不變；`TryApplyCh2Ripple` 後恰好 +3，總計一次。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter2Quest.h`，`game/dialog/DialogOpener.h`、`DialogSource.h`、`DialogState.h`，`game/entities/Player.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純邏輯測試）

## OO 概念與設計重點

「一次性鍵」模式（設旗標防止重複結算）是跨章 karma 漣漪的核心機制，確保每個選擇的業力效應在整個學期只累積一次，而非每次對話都重複。`ScoldedSenior` 的 karma 中性（不扣分）體現了設計決策：指正他人屬理性發言，不是壞行為，Ch2 的「保持距離」只是社交後果，無 karma 懲罰。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch2_ripple.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch2_ripple.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
