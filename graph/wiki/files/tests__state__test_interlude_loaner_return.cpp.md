---
id: file:tests/state/test_interlude_loaner_return.cpp
type: test
path: tests/state/test_interlude_loaner_return.cpp
domain: tests
bucket: state
loc: 172
classes: []
sources: ["tests/state/test_interlude_loaner_return.cpp"]
---
# `test_interlude_loaner_return.cpp`

> **一句定位**：驗證「歸還管理員借傘」的完整語義：僅在 Ch2→Ch3 市集 + 正確 NPC + 持有借傘時 +10 karma（冪等），不授結局傘旗標；地標只在正確條件下生成一次；跳過歸還是安全的。

## 職責

此測試檔為「可選責任感選項」的 `TryReturnLibrarianUmbrella` 函式提供四個面向的驗證：

1. **基本邏輯 + 冪等性**：持有 `kFlagLibrarianUmbrella` 時，在 Ch2→Ch3 市集對 `kNpcLibrarianReturn` 互動 → karma +10、傘清除（`HeldUmbrellaKind()==None`、`HasUmbrella()==false`、旗標清除）、設 `kFlagLibrarianUmbrellaReturned`。關鍵：`kFlagHasTrueUmbrella` 不被設（借傘不解鎖 Ending A）。第二次呼叫不再給分（冪等）。

2. **條件守門**：四個 SUBCASE 分別驗證 returnTo 不對（Ch2 而非 Ch3）/ 狀態不對（非幕間）/ NPC id 不對（`shop_auntie`）/ 未持傘時皆不給分、不誤設旗標。

3. **地標生成**：使用 `World`；Ch1（非幕間）→ 無地標；Ch1→Ch2 市集（returnTo=Ch2）+ 強制持傘 → 仍無地標（目的地判定擋下）；Ch2→Ch3 市集（returnTo=Ch3）+ 持傘 → 地標生成一次，重複呼叫為無操作；生成後 Player 仍在 `Objects().front()`。

4. **跳過是安全的**：在幕間中持有借傘但不歸還，karma 仍是 50（起始值），`kFlagLibrarianUmbrellaReturned` 為 false（無業力欠債，純可選）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::TryReturnLibrarianUmbrella(EventBus, Player&, npcId, state, returnTo)`：被測函式。
- `nccu::kNpcLibrarianReturn`：歸還點 NPC 的 id 常數。
- `GiveLoaner(Player&)`：`SetHasUmbrella(true)` + `SetFlag(kFlagLibrarianUmbrella)` 的 helper。
- `HasReturnMarker(const World&)`：在 `Objects()` 中查找 `kNpcLibrarianReturn` NpcId 的 helper。
- `World::MaybeSpawnInterludeLibrarianReturn()`：一次性地標生成（被測方法）。
- 旗標：`kFlagLibrarianUmbrella`、`kFlagLibrarianUmbrellaReturned`、`kFlagHasTrueUmbrella`。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`Chapter2Quest.h`（`TryReturnLibrarianUmbrella`、`kNpcLibrarianReturn`）、`EventBus.h`、`NPC.h`、`Player.h`、`GameObject.h`、`World.h`、`SemesterState.h`、`SemesterStateMachine.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 E 互動路徑（`RunInteractHooks` 中的 `TryReturnLibrarianUmbrella`）與 World 延後生成機制。

## OO 概念與設計重點

「借傘不解鎖 Ending A」的明確斷言（`CHECK_FALSE(p.HasFlag(kFlagHasTrueUmbrella))`）是防止未來混淆兩種傘旗標的重要守門。「跳過安全性」TEST_CASE 體現了「可選選項不得成為卡關點」的設計原則，與 `test_ending_gate.cpp` 中的防卡關測試哲學一致。地標生成的 Player 不變量確認是整個 World 物件管理安全性的縱深防禦。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/state/test_interlude_loaner_return.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/state/test_interlude_loaner_return.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
