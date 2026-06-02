---
id: "file:include/game/dialog/DialogOpener.h"
type: header
path: include/game/dialog/DialogOpener.h
domain: game
bucket: dialog
loc: 71
classes: []
sources: ["include/game/dialog/DialogOpener.h"]
---
# `DialogOpener.h`

> **一句定位**：把 `(npcId, SemesterState[, Player])` 對映到「開啟哪段對白/選單」的路由層——連結 `DialogSource` 的解析資料與 `DialogState` 的執行期會話。

## 職責

`DialogOpener.h` 屬 game dialog 層，是遊戲對白系統的「路由中心」。它依任務旗標、業力和學期狀態，決定 NPC 在給定時刻應顯示哪個子段（subState），並在分支 NPC 的情況下組出選單。

標頭宣告四個自由函式，呈現清晰的職責層次：

1. **`OpenNpcDialogSub(dlg, npcId, state, subState)`**：純台詞開場，直接以指定的 `subState` 子段開啟，不帶選項。找不到子段時為 no-op。

2. **`OpenNpcDialog(dlg, npcId, state)`**（三參數版）：智慧開場——組出 `subState=0` 的開場台詞，並為在此狀態分支的 NPC（具有 subState ≥1 的子段）加上 `DialogChoice` 選項（label/karma/flag 取自各子段，`nextLines` 為後果台詞）。無分支時維持純台詞。

3. **`ResolveOpenerSubState(npcId, state, player)`**：依任務旗標決定開場 subState（純函式，無副作用）。例如 Ch1 ta 在 `Flag_FoundForm` 後→1，victim 在 `Flag_PromisedVictim` 後→1（recap）；其餘→0。

4. **`OpenNpcDialog(dlg, player, npcId, state)`**（四參數版，`GameController` 實際呼叫的版本）：先呼叫 `ResolveOpenerSubState` 解析開場 subState；為 0 時委派三參數版（保留選單路徑）；≥1 時以純台詞開啟並「僅一次」施加其 karma/flag（防護：只在 subState 要設定玩家尚未持有的 true 旗標時套用）。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `OpenNpcDialogSub(dlg, npcId, state, subState)` | 純台詞開場；找不到時 no-op。 |
| `OpenNpcDialog(dlg, npcId, state)` | 智慧開場：subState=0 台詞 + 分支 NPC 的選單組裝。 |
| `ResolveOpenerSubState(npcId, state, player)` | 依旗標/業力/狀態解析開場 subState；純函式。 |
| `OpenNpcDialog(dlg, player, npcId, state)` | GameController 實際呼叫的版本；含一次性副作用防護。 |
| `Player` | 全域命名空間前向宣告（讀旗標/業力）。 |
| `nccu::DialogState` | 前向宣告。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/game/state/SemesterState.h`（路由依賴的 FSM 狀態型別）；`Player` 為前向宣告，無需拉入標頭。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（四參數版）、`src/game/controller/InteractDispatch.cpp`（互動觸發時呼叫）、`src/game/controller/screens/DialogScreen.cpp`（選單確認後的選項 nextLines 播放）、`src/game/dialog/DialogOpener.cpp`（實作）；大量測試（`test_dialog_opener`、所有 `test_ch*_quest`、`test_ch*_ripple`、`test_suit_senior_oneshot` 等）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：遊戲對話資料層的路由進入點；在 Controller 的 `DispatchInteract` 或 `InteractDispatch` 觸發互動後、開啟 `DialogState` 之前被呼叫。

## OO 概念與設計重點

四個函式形成了**Layered API**設計：從最低層（`OpenNpcDialogSub`，直接指定子段）到最高層（四參數 `OpenNpcDialog`，完整路由），呼叫端按需選擇適合的層次。`GameController` 使用最高層；測試可直接呼叫最低層驗證特定子段，無需模擬任務旗標。

`ResolveOpenerSubState` 作為**純函式**單獨宣告，允許測試在不開啟 `DialogState` 的情況下驗證路由邏輯——這是「把副作用（開啟對話）和決策（解析子段）分離」的 SRP 應用。

四參數版的「一次性副作用防護」邏輯和 `ApplyDialogChoice` 的防護類似，但發生在「路由決策時」而非「選項確認時」，防護邏輯與 `DialogChoiceApply.h` 的保護層各司其職，互補而不重複。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogOpener.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogOpener.h) · [← 全檔索引](../files-index.md)
