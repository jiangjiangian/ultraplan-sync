---
id: "file:tests/dialog/test_dialog_loader.cpp"
type: test
path: tests/dialog/test_dialog_loader.cpp
domain: tests
bucket: dialog
loc: 99
classes: []
sources: ["tests/dialog/test_dialog_loader.cpp"]
---
# `test_dialog_loader.cpp`

> **一句定位**：以 fixture markdown（`dialog_sample.md`）驗證 `DialogLoader::LoadChapter` 的解析正確性——NPC 區段切分、子狀態升序排列、台詞內容、空子區塊保留、及各 metadata 欄位的預設值。

## 職責

本檔依賴 `TEST_FIXTURES_DIR` 指向 fixture 目錄，使用 `dialog_sample.md` 作為受控的測試素材（不使用真正的章節 markdown，避免素材變動導致測試失敗）。包含一個大型 `TEST_CASE`，逐一驗證解析器的各項行為。

**NPC 區段識別**：樣本含兩位 NPC（學長、學妹），NPC 前後的非 NPC 標題（`## 章節 metadata`、`## 場景旁白`）被正確排除。`chapter.npcs.size() == 2`。

**子狀態升序排列**：學長有 2 個子狀態（subState 0 和 1）、學妹有 2 個（subState 0 和 2，中間的 1 不存在）。各項目依 `subState` 整數升序排列。

**台詞內容**：學長 subState 0（2 行，ASCII 引號）：`"嗨，學弟。"` 與 `"你也來測試嗎？"`；subState 1（1 行）：`"又見面了。"`。學妹 subState 0（2 行，CJK 引號）：`"學長你好。"` 與 `"今天天氣真好。"`。

**空子區塊保留**：學妹 subState 2（`"沒台詞的 substate"`）無任何 `- "line"` 項目，仍產生 `lines.empty()` 的 `SubEntry`，確認 `LoadChapter` 不丟棄空子區塊。

**metadata 預設值**：樣本無 karma 或 `Flag_X =` 標註，`karmaDelta == 0`、`setsFlag == ""`、`flagValue == false`；`choiceLabel` 等於標題文字本身（無 `「…」` 覆寫）。

`Find(subs, subState)` helper：在 `SubEntry` 向量中依 `subState` 整數查找，找不到回傳 nullptr。

## 關鍵內容（類別 / 函式 / 資料）

- `nccu::dialog::LoadChapter(path)`（被測函式）：讀取 markdown，回傳含 `npcs` map 的 `Chapter` 結構。
- `nccu::dialog::SubEntry`：含 `subState`、`lines`、`choiceLabel`、`karmaDelta`、`setsFlag`、`flagValue`。
- `Find(subs, subState)`：測試 helper，依子狀態整數查找。
- `FixturePath(name)`：拼接 `TEST_FIXTURES_DIR + "/" + name`。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/dialog/DialogLoader.h`（被測主體）
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純解析器單元測試）

## OO 概念與設計重點

本檔示範了「以受控 fixture 素材」而非「真實出貨素材」測試解析器的最佳實踐：`dialog_sample.md` 的結構由測試設計者控制，不會因作者修改台詞而導致案例失敗。這也讓測試可以覆蓋邊界案例（空子區塊、CJK 引號）而不依賴出貨素材恰好包含這些形式。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/dialog/test_dialog_loader.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/dialog/test_dialog_loader.cpp) · [← 全檔索引](../files-index.md)
