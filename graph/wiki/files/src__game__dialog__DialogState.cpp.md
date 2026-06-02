---
id: file:src/game/dialog/DialogState.cpp
type: source
path: src/game/dialog/DialogState.cpp
domain: game
bucket: dialog
loc: 92
classes: []
sources: ["src/game/dialog/DialogState.cpp"]
---
# `DialogState.cpp`

> **一句定位**：對話框的執行期狀態機——管理台詞游標、分頁、選項游標，並在確認時推進或轉入分支後果台詞。

## 職責

`DialogState` 是對話系統的 Model 核心，儲存一段對話的完整執行期狀態（台詞列表、選項列表、台詞游標、頁游標、選項游標、啟用旗標、NPC context id），並提供讀取與推進的介面。

**開啟（`Open`）**：接受台詞 vector 與可選的選項 vector。空台詞輸入為 no-op（不啟動）。重置所有游標為 0，設 `active_=true`。

**讀取當前內容**：`CurrentLine()` 回傳 `lines_[cursor_]`；`CurrentPageRows()` 呼叫 `dialog::LayoutPages` 計算當前台詞的分頁輸出，取 `pageCursor_` 指定頁；`CurrentLineHasMorePages()` 判斷本台詞是否尚有後頁。`HasMore()` 綜合判斷：本行有後頁 or 後有台詞 or 後有選項，供 View 決定是否畫 `▼` 提示。

**推進（`Advance`）**：
- 若本行尚有後頁：`++pageCursor_`，維持台詞不動。
- 否則 `++cursor_`，重置 `pageCursor_=0`。
  - 若還有台詞：返回 `nullptr`。
  - 無台詞且無選項：`Close()`，返回 `nullptr`。
  - 無台詞有選項：進入選單模式（cursor_ 停在 lines_.size()）。
- 選單模式確認：複製 `choices_[choiceCursor_]` 到 `picked_`，若該選項有 `nextLines`，則以後果台詞替換 `lines_`、清空 `choices_`、重置游標（active 維持 true，繼續播放後果台詞）；否則 `Close()`。回傳 `&picked_` 指標（指向複製後的成員，避免 Close 後 use-after-free）。

**選項游標（`MoveChoice`）**：以 `std::clamp` 夾制在 [0, choices_.size()-1]，不繞回。

**關閉（`Close`）**：清除所有狀態，`npcId_` 亦清空。

## 關鍵內容（類別 / 函式 / 資料）

- `DialogState::Open(vector<string>, vector<DialogChoice>)`：開啟對話，空台詞為 no-op。
- `DialogState::CurrentLine() const -> const string&`：當前台詞原文（未分頁）。
- `DialogState::CurrentPageRows() const -> vector<string>`：當前台詞的當前頁列陣列（已斷行/分頁）。
- `DialogState::CurrentLineHasMorePages() const -> bool`：本行是否尚有後頁。
- `DialogState::HasMore() const -> bool`：是否還有後續內容（後頁/後台詞/後選項）。
- `DialogState::MoveChoice(int delta) noexcept`：移動選項游標（夾制）。
- `DialogState::Advance() -> const DialogChoice*`：推進對話，選單確認時回傳選項指標。
- `DialogState::Close() noexcept`：完整重置。
- `dialog::kBoxCells` / `dialog::kBoxRowsPerPage`：排版常數，引自 `DialogLayout.h`。

## 相依與在架構中的位置

- **#include（往外）**：`DialogState.h`（宣告及 `DialogChoice` 型別）、`DialogLayout.h`（`LayoutPages` / `kBoxCells` / `kBoxRowsPerPage`）；標準庫 `<algorithm>`。
- **被誰使用（往內）**：`DialogView.cpp`（讀取顯示內容）、`DialogOpener.cpp`（呼叫 `Open` / `SetNpcContext`）、`Chapter1Quest.cpp` / `Chapter2Quest.cpp`（`dialog.Active()` 閘控章節通關）、`DialogState.h` 的所有引入方（`GameController` 等）。
- **繼承 / 實作 / 體現**：—（具體狀態機物件，非介面）。
- **每幀管線 / MVC 角色**：Model 層。`GameController` 在每幀管線的互動與推進階段讀寫 `DialogState`；View（`DialogView`）只讀。

## OO 概念與設計重點

`DialogState` 是 [State 模式](../concepts/pat-state.md) 的精簡實作：對話框的「台詞中 / 選單 / 關閉」三個邏輯狀態由 `active_`、`cursor_ >= lines_.size()`、`choices_` 三個資料位元聯合描述，`Advance` 以線性邏輯而非子類別完成轉換。`Advance` 回傳 `const DialogChoice*`（指向 `picked_` 成員複製），是規避 close 後 use-after-free 的防禦性設計。`LayoutPages` 在 `CurrentPageRows` 每次呼叫時重新計算，不快取——對話框開啟期間台詞不可變，計算純粹。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogState.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogState.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
