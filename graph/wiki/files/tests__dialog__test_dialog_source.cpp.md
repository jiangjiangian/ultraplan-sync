---
id: "file:tests/dialog/test_dialog_source.cpp"
type: test
path: tests/dialog/test_dialog_source.cpp
domain: tests
bucket: dialog
loc: 134
classes: []
sources: ["tests/dialog/test_dialog_source.cpp"]
---
# `test_dialog_source.cpp`

> **一句定位**：驗證執行期的 `DialogSource` 供給層——英文 npcId 對應到中文 NPC 區段、SemesterState 對應到章節檔、快取行為、找不到時回傳空且不丟例外，以及 `Reload()` 重建快取。

## 職責

本檔依賴 `TEST_CONTENT_DIR` 讀取真正的 `docs/content/chapter1.md`，驗證 `DialogSource` 對出貨素材的解析結果。包含 4 個 `TEST_CASE`：

**Ch1 西裝學長四個子狀態的 metadata**：`Entries("suit_senior", Chapter1_AddDrop)` 回傳 4 個子狀態；逐一驗證：subState 0（開場白）`choiceLabel == "初次接觸"`、5 行台詞開頭正確；subState 2 (c)（接受取傘）`karmaDelta == 0`、`setsFlag == kFlagScoldedSenior`、`flagValue == false`、`choiceLabel == "接受，取傘後交給學長"`；subState 3 (d)（點破疑點）`karmaDelta == 5`、`setsFlag == kFlagHelpedSenior`、`flagValue == true`、`choiceLabel == "點破傘的疑點，轉而提供正規協助"`。

**Ch1 助教獎勵子狀態 metadata**：`Entries("ta", Chapter1_AddDrop)` 回傳 3 個；subState 1 驗證 `karmaDelta == 5`、`setsFlag == kFlagHelpedTACh1`、`flagValue == true`、`choiceLabel == "玩家完成助教的跑腿請求後"`。

**找不到時回傳空**：有效章節下查未知英文 id（`"does_not_exist"`）回傳空向量；已知 id 但在結局章節（`Ending_A`）查詢也回傳空——結局檔不含 NPC 對話區段。均不丟例外。

**`Reload()` 重建快取**：先填入快取，呼叫 `Reload()`，再次查詢結果以值相同（`choiceLabel`、`karmaDelta`）但位址不同（重新從磁碟讀取）。確認 `suit_senior` (c) 的 `karmaDelta == 0`（現行值）。

`Find(subs, subState)` helper：依整數查找 `SubEntry`。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::dialog::SetContentDir(path)`：設定解析根目錄。
- `nccu::dialog::Entries(npcId, SemesterState)`：按需載入並快取章節內容，回傳 `SubEntry` vector 的 const 參考。
- `nccu::dialog::Reload()`：清除快取，下次呼叫重新從磁碟讀取。
- `nccu::dialog::SubEntry`：含 `subState`、`lines`、`choiceLabel`、`karmaDelta`、`setsFlag`、`flagValue`。
- 各旗標常數：`kFlagScoldedSenior`、`kFlagHelpedSenior`、`kFlagHelpedTACh1`。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogSource.h`（被測主體）、`include/game/quest/Flags.h`、`include/game/state/SemesterState.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料供給層單元測試）

## OO 概念與設計重點

本檔與 `test_dialog_loader.cpp` 的分工明確：`test_dialog_loader` 以 fixture 素材驗證解析器邏輯（不受出貨素材影響），`test_dialog_source` 以真實出貨素材驗證供給層的完整 metadata（確保章節內容正確）。這釘住了 `chapter1.md` 中具體的 karma 數值與旗標名稱，防止作者在 markdown 中意外改動這些關鍵值。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_source.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_source.cpp) · [← 全檔索引](../files-index.md)
