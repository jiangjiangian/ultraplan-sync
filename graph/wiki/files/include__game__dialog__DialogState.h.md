---
id: "file:include/game/dialog/DialogState.h"
type: header
path: include/game/dialog/DialogState.h
domain: game
bucket: dialog
loc: 103
classes: [DialogChoice, DialogState]
sources: ["include/game/dialog/DialogState.h"]
---
# `DialogState.h`

> **一句定位**：遊戲對話會話的純資料核心——由 `World` 持有，View 以 `const` 讀取，`GameController` 推進；支援台詞分頁、選單模式和 NPC 歸屬標注，無 raylib。

## 職責

`DialogState.h` 屬 game dialog 層，定義了對話系統的「執行期會話」資料結構，是整個對話系統中使用頻率最高的核心型別（被 50+ 個不同檔案引用）。它位於 MVC 架構的 Model 層（由 `World` 持有），純資料，不含 raylib 或輸入。

**`DialogChoice`**：一個可選分支的完整資料：`label`（選單標籤）、`karmaDelta`（業力增減）、`setsFlag`/`flagValue`（旗標副作用）、`nextLines`（選取後播放的後果台詞）。

**`kDialogExitLabel = "我再想想…"`**：能退出的選項標籤哨兵。帶有此標籤的選項表示「無任何狀態變動地結束」，確認時略過針對該 NPC 的選後記帳（如 Ch4 助教的 `Flag_TaFinaleChoiceMade` 自鎖）。

**`DialogState`** 的核心狀態機：
- **台詞模式**（`active_=true, cursor_<lines_.size()`）：逐行播放；每行可能有多頁（`pageCursor_` 分頁游標）；`Advance()` 先翻頁，翻完再進下一行；越過最後一行且無選項時 `Close()`，有選項時進入選單模式。
- **選單模式**（`AtChoice()==true`）：`choiceCursor_` 高亮、`MoveChoice(delta)` 移動；`Advance()` 確認高亮選項 → 回傳穩定指標（`picked_`）；若選項帶 `nextLines` 則回到台詞模式播放後果，否則 `Close()`。
- **「先顯示、再前進」語意**：`Open()` 顯示第 0 行；`Advance()` 移到下一狀態（翻頁/下一行/確認）。

`SetNpcContext(string npcId)`/`NpcId()`：標注對話歸屬 NPC（供一次性防護讀取）；`Close()` 時清空。`picked_` 是穩定儲存的成員，讓 `Advance()` 回傳的指標在 `Close()` 後仍可安全讀取。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `struct DialogChoice` | label/karmaDelta/setsFlag/flagValue/nextLines；選項完整 payload。 |
| `kDialogExitLabel = "我再想想…"` | 能退出的選項哨兵標籤；確認時略過記帳。 |
| `class DialogState` | 純資料對話會話；由 `World` 持有。 |
| `Open(lines, choices)` | 開啟會話；清空游標、設定行與選項。 |
| `Active()` | 是否正在進行對話。 |
| `AtChoice()` | 是否處於選單模式。 |
| `CurrentLine()` | 當前台詞行文字（const 參考）。 |
| `CurrentPageRows()` | 當前行的當前頁各列（依 `kBoxCells`/`kBoxRowsPerPage` 分頁）。 |
| `CurrentLineHasMorePages()` | 當前行是否還有下一頁（驅動「▼」翻頁提示）。 |
| `HasMore()` | 前進是否仍停留在同一段對話內（適用「▼ 更多」提示）。 |
| `Choices()` | const 存取選項向量。 |
| `ChoiceCursor()` | 目前高亮選項索引。 |
| `MoveChoice(delta)` | 移動選單游標（夾限合法範圍）。 |
| `SetNpcContext(string)` / `NpcId()` | 標注/查詢 NPC 歸屬。 |
| `Advance()` | 推進一步；選單模式確認時回傳穩定的 `const DialogChoice*`。 |
| `Close()` | 結束對話；重設所有游標。 |

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫（`<cstddef>`、`<string>`、`<vector>`）。完全不依賴 raylib。
- **被誰使用（往內）**：`include/game/dialog/DialogView.h`（View 繪製）、`include/game/world/World.h`（Model 持有）；幾乎所有 Controller 和 engine/platform 的檔案都引入此標頭（50+ 引用），是系統中相依度最高的標頭之一。
- **繼承 / 實作 / 體現**：—（純資料，無繼承）
- **每幀管線 / MVC 角色**：**MVC 的 Model**——`World` 的成員；View 以 `const DialogState&` 讀取繪製，Controller 以 `DialogState&` 推進會話（`Advance()`/`Close()`/`MoveChoice()`）。

## OO 概念與設計重點

`DialogState` 是一個精心設計的**State Machine（小規模）**：`active_`、`cursor_`、`pageCursor_`、`choiceCursor_` 四個游標共同定義了對話的當前狀態，`Advance()` 是唯一的狀態轉移函式，覆蓋了所有分支（翻頁/下一行/進入選單/確認/關閉），且回傳值明確指示轉移結果（`nullptr` vs `const DialogChoice*`）。

`picked_` 的「穩定儲存」設計是一個微妙但重要的安全措施：`Advance()` 回傳指向 `picked_`（成員，不是 `choices_` 內某元素）的指標，確保即使在 `Close()` 清空 `choices_` 後，呼叫端讀取 `karmaDelta`/`setsFlag` 等欄位仍是安全的，避免了 use-after-free。

「先顯示、再前進」的語意設計與大多數對話系統（常見的是「顯示即推進」）相反，刻意為之：`Open()` 展示第 0 行，讓玩家有機會閱讀；按下按鍵時才 `Advance()` 移動。這使每個狀態轉移都對應一個明確的玩家動作，不會有「剛開啟就跳過」的問題。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/dialog/DialogState.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/dialog/DialogState.h) · [← 全檔索引](../files-index.md)
