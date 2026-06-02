---
id: "file:tests/dialog/test_dialog_state.cpp"
type: test
path: tests/dialog/test_dialog_state.cpp
domain: tests
bucket: dialog
loc: 136
classes: []
sources: ["tests/dialog/test_dialog_state.cpp"]
---
# `test_dialog_state.cpp`

> **一句定位**：驗證 `DialogState` 狀態機的完整生命週期——空台詞空操作、逐行推進、選項模式切換、游標裁切與確認、帶後續台詞的選項、確認指標的 UAF 安全性，以及 `ApplyDialogChoice` 對玩家的影響。

## 職責

本檔為 `DialogState`（對話狀態機）的核心合約提供完整的單元測試，包含 8 個 `TEST_CASE`，全程無 GL、無 harness、不依賴任何外部素材。

**空台詞空操作**：`d.Open({})` 後 `Active() == false`，不啟動對話框。

**先顯示後推進**：`Open({"a","b","c"})` 後 `CurrentLine() == "a"`，不跳過第一行。

**逐行推進後關閉**：`Advance()` 從 a→b（回傳 nullptr），再 Advance() 關閉（`Active() == false`）。

**未啟用時 `CurrentLine()` 安全**：不崩潰，回傳空字串（無 UB）。

**台詞後進入選項模式**：`Open({"intro"}, {{"refuse",0},{"accept",-5}})` 後 Advance 一次越過 `"intro"`，進入選項模式（`AtChoice() == true`、`Choices().size() == 2`、`ChoiceCursor() == 0`）。

**游標裁切與確認**：`MoveChoice(-1)` 裁切於 0；`MoveChoice(1)` 後再 `MoveChoice(1)` 裁切於 1（不越界）；Advance 回傳被選中者的 `DialogChoice*`（label/karmaDelta/setsFlag 正確），且立刻關閉（`Active() == false`，無後續台詞）。

**帶 `nextLines` 的選項**：選項 0 帶後續台詞 `{"c0","c1"}`，Advance 確認後 `Active() == true`、`CurrentLine() == "c0"`；再 Advance 到 c1，再關閉。

**確認指標 UAF 安全**：`Advance()` 選項確認並 `Close()` 後，回傳的 `DialogChoice*` 在 `Active() == false` 的狀態下仍可安全讀取（`karmaDelta`、`setsFlag`、`flagValue`）。這釘住了「`Close` 不得釋放 choices_ 中被回傳指標指向的物件」的保證（實作以 `lastPicked_` 持有副本）。

**`ApplyDialogChoice` 語意**：`karmaDelta == -5` + `flagValue == false` → 扣 karma 且 `ClearFlag`；`karmaDelta == +10` + `flagValue == true` → 加 karma 且 `SetFlag`。

## 關鍵內容（類別 / 函式 / 資料）

- `DialogState::Open(lines)`、`Open(lines, choices)`：啟動對話框。
- `DialogState::Active()`、`AtChoice()`、`CurrentLine()`、`ChoiceCursor()`、`Choices()`。
- `DialogState::Advance()` → `const DialogChoice*`：推進並回傳（選項確認時非 null）。
- `DialogState::MoveChoice(delta)`：游標移動（自動裁切）。
- `nccu::ApplyDialogChoice(Player&, DialogChoice&)`：套用 karma delta 與旗標。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogState.h`、`include/game/controller/DialogChoiceApply.h`、`include/game/entities/Player.h`、`include/game/quest/Flags.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：驗證 Model 層的對話狀態機（`DialogState` 屬 `World` 的一部分）

## OO 概念與設計重點

`DialogState` 是一個小型狀態機，在「播放中」（逐行）、「選項模式」、「後續台詞」、「關閉」之間切換。UAF 安全 case 是必要的回歸測試：確認呼叫端（`GameController`）在 `Active() == false` 後讀取選項 metadata 是安全的，這是典型的 C++ 物件生命週期邊界條件。`ApplyDialogChoice` 測試將「選項選了什麼」與「玩家受到什麼影響」解耦，符合單一職責。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_state.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_state.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[pat-state](../concepts/pat-state.md)
