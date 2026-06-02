---
id: "file:tests/dialog/test_dialog_opener.cpp"
type: test
path: tests/dialog/test_dialog_opener.cpp
domain: tests
bucket: dialog
loc: 338
classes: []
sources: ["tests/dialog/test_dialog_opener.cpp"]
---
# `test_dialog_opener.cpp`

> **一句定位**：驗證 `DialogOpener` 系列函式的完整行為——依 npcId／章節載入對話、未知 id 不啟用、選項分支後續台詞、once-only 獎勵守門、以及 `ResolveOpenerSubState` 依旗標決定開場子狀態。

## 職責

本檔是 `DialogOpener` 最完整的測試套件，包含 15 個 `TEST_CASE`，覆蓋三個面向：

**`OpenNpcDialogSub` 三參數版**：依 npcId + `SemesterState` + subState 整數載入單一子狀態的台詞，不帶選項。驗證苦主 Ch1 (a) 開場白的行內容（`"……我的傘也不見了。"` 開頭、共 5 行後關閉）；未知 npcId 時 `Active() == false`；西裝學長 Ch1 (a) 第一行正確。

**`OpenNpcDialog` 三參數版**：載入完整的開場白 + 選項選單。苦主 Ch1：走過 5 行開場白後 `AtChoice() == true`、兩個選項（「我去幫你追」/ 「別過頭，當作沒看見」）；選擇分支 0 回傳 `c->setsFlag == kFlagPromisedVictim`，且後續播放（b）的台詞 `"真的？謝謝你……"`。福利社阿姨 Ch1：4 行開場白後 3 個選項（詢問雨傘／購買醜綠傘／請阿姨喝咖啡），購買選項 `setsFlag == ""`（Ch1 阿姨購傘是敘事種子，不設旗標）。

**一次性獎勵守門**：「請阿姨喝咖啡」設 `kFlagBoughtCoffeeForAuntie` + karma +5；同一玩家第二次重選，`ApplyDialogChoice` 的守門使 karma 停在 `k0+5` 不再增加（`k0+5 ≠ k0+10`）。惰性選項（詢問雨傘，karma +0、無旗標）可重選且不動 karma。

**`ResolveOpenerSubState`**：助教依 `kFlagFoundForm` / `kFlagHelpedTACh1` 決定 subState（0→1→1）；苦主依 `kFlagPromisedVictim` / `kFlagHasTrueUmbrella` 決定（0→1→3，傘旗標優先於承諾旗標）；非任務 NPC（bookworm）永遠回傳 0。

**Player overload（四參數版）**：助教在 `kFlagFoundForm` 已設時開啟獎勵 subState，套用 karma +5 與 `kFlagHelpedTACh1`；重複呼叫時守門略過（不重複）。苦主在無旗標時仍呈現兩選項；在已有承諾旗標時是純台詞回顧（無選項，karma 不變）。

**西裝學長 Ch1 (b) 指正分支**：需先設 `kFlagPromisedVictim` 才呈現選項選單；選項 0 是「理性指出他品行不該，要回雨傘」（`karmaDelta == 3`、設 `kFlagScoldedSenior`）；`ApplyDialogChoice` 端到端確認 karma 和旗標同步。Ch2 西裝學長在 `kFlagScoldedSenior` 時導向 (c) 冷淡分支（`ResolveOpenerSubState == 2`）。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::OpenNpcDialogSub(DialogState&, npcId, SemesterState, subState)`：載入單子狀態（無選項）。
- `nccu::OpenNpcDialog(DialogState&, npcId, SemesterState)`：載入開場白 + 選項清單（3 參數）。
- `nccu::OpenNpcDialog(DialogState&, Player&, npcId, SemesterState)`：Player overload，套用 once-only 獎勵（4 參數）。
- `nccu::ResolveOpenerSubState(npcId, SemesterState, Player&)`：依旗標決定開場子狀態整數。
- `nccu::ApplyDialogChoice(Player&, DialogChoice&)`：套用 karma 與旗標（守門）。
- 各旗標常數：`kFlagPromisedVictim`、`kFlagBoughtCoffeeForAuntie`、`kFlagHelpedTACh1`、`kFlagScoldedSenior`、`kFlagHasTrueUmbrella`、`kFlagHelpedSenior`。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogOpener.h`、`include/game/dialog/DialogState.h`、`include/game/controller/DialogChoiceApply.h`、`include/game/entities/Player.h`、`include/game/quest/Flags.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：驗證 Controller 層的 E 互動路由（NPC 對話開啟、選項套用）

## OO 概念與設計重點

本檔完整釘住了 `DialogOpener` 的三層行為：載入（`LoadChapter`）、路由（`ResolveOpenerSubState`）、套用（`ApplyDialogChoice` with once-only 守門）。once-only 守門是「幂等寫入」的設計模式——選項標記的玩家狀態只在第一次寫入，後續重複呼叫為 no-op，防止重複刷取 karma 的 exploit。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_opener.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_opener.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-mvc](../concepts/arch-mvc.md)
