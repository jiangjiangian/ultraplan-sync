---
id: "file:tests/entities/test_npc_loaddialog.cpp"
type: test
path: tests/entities/test_npc_loaddialog.cpp
domain: tests
bucket: entities
loc: 38
classes: []
sources: ["tests/entities/test_npc_loaddialog.cpp"]
---
# `test_npc_loaddialog.cpp`

> **一句定位**：驗證 `NPC::LoadDialog` 能依 npcId 與章節載入對話台詞，以及 `GameObject::DialogLines()` 虛擬函式暴露 NPC 台詞的正確性。

## 職責

本檔包含 3 個 `TEST_CASE`，驗證 `NPC` 的動態對話載入介面。本檔依賴 `test_dialog_content_dir.cpp` 的靜態初始化器設定 content dir，因此 `LoadDialog` 能讀到真正的章節 markdown。

**成功載入**：`npc.LoadDialog("suit_senior", SemesterState::Chapter1_AddDrop, 0)` 後，`DialogLineCount() == 5`、`CurrentLineText() == "欸，加退選也沒搶到嗎？"`（西裝學長 Ch1 (a) 開場白第 0 行）。

**找不到時空白**：`LoadDialog("nobody", SemesterState::Ending_A, 0)` 後 `DialogLineCount() == 0`，不崩潰。

**`GameObject::DialogLines()` 虛擬分派**：以 `GameObject& as_base = npc`（基底類別參考）呼叫 `as_base.DialogLines()`，回傳 `const vector<string>*`；驗證 `lines->size() == 2`、`(*lines)[0] == "hi"`。這釘住了「透過基底類別的對話介面」（異質容器存取路徑）。

## 關鍵內容（類別 / 函式 / 資料）

- `NPC::LoadDialog(npcId, SemesterState, subState)`：從 `DialogSource` 載入指定子狀態的台詞。
- `NPC::DialogLineCount()`、`NPC::CurrentLineText()`：台詞查詢介面。
- `GameObject::DialogLines()` → `const vector<string>*`：虛擬函式，暴露 NPC 的台詞供 View 或 controller 讀取。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/NPC.h`、`include/engine/core/GameObject.h`、`include/game/state/SemesterState.h`、`include/engine/math/Vec2.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純 Model 層單元測試）

## OO 概念與設計重點

`DialogLines()` 的虛擬函式 case 釘住了「基底類別指標訪問子類別資料」的多型契約：`World` 以 `unique_ptr<GameObject>` 儲存所有物件，`GameController` 若需讀取 NPC 台詞必須透過此虛擬介面。確保這個介面在重構後仍正確回傳，是避免 View 層因 `DialogLines() == nullptr` 而空繪的回歸保護。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_npc_loaddialog.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_npc_loaddialog.cpp) · [← 全檔索引](../files-index.md)
