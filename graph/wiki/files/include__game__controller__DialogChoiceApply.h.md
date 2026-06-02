---
id: "file:include/game/controller/DialogChoiceApply.h"
type: header
path: include/game/controller/DialogChoiceApply.h
domain: game
bucket: controller
loc: 29
classes: []
sources: ["include/game/controller/DialogChoiceApply.h"]
---
# `DialogChoiceApply.h`

> **一句定位**：將已確認對話選項的副作用（業力增減＋旗標寫入）套用到玩家的自由函式宣告，設計為獨立可單元測試的接縫。

## 職責

此標頭宣告一個自由函式 `ApplyDialogChoice(Player& player, const DialogChoice& choice)`，職責是在玩家確認了一個對話選項後，把該選項的副作用（`karmaDelta` 業力增減、`setsFlag`/`flagValue` 旗標寫入）套用到玩家身上。

設計為自由函式是刻意的：使其不需啟動完整的 Controller 輸入迴圈就能單元測試，讓測試可直接呼叫函式並斷言玩家狀態，而不依賴整個 `GameController::Update()` 流程。

此函式包含一個關鍵的防護邏輯：**一次性自旗標選項的冪等保護**。某些選項（如福利社阿姨的「請咖啡」）可多次進入，若無防護每次都能重複刷取業力。防護邏輯如下：若選項欲設定的旗標玩家**已持有**（代表獎勵已領過），則略過業力與旗標寫入；清除型選項（`flagValue=false`）或玩家尚未持有的旗標則正常套用。`suit_senior`/`ta` 的一次性防護靠「不再重現選單」在結構上防護；福利社阿姨的防護改放在此函式內。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `ApplyDialogChoice(Player& player, const DialogChoice& choice)` | 自由函式；套用業力增減與旗標寫入；含一次性選項的冪等保護邏輯。 |
| `Player` | 全域命名空間的模型物件，前向宣告以避免拉入整份 `Player.h`。 |
| `nccu::DialogChoice` | 包含 `karmaDelta`、`setsFlag`、`flagValue` 的選項資料結構，前向宣告。 |

## 相依與在架構中的位置

- **#include（往外）**：僅前向宣告（無 `#include`），保持標頭的最小相依性。
- **被誰使用（往內）**：`src/game/controller/DialogChoiceApply.cpp`（實作）、`src/game/controller/screens/DialogScreen.cpp`（確認選項時呼叫）、`tests/dialog/test_dialog_opener.cpp`、`tests/dialog/test_dialog_state.cpp`（直接測試此函式）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層的輔助函式；在 `HandleDialog` 確認對話選項時被呼叫，屬於每幀對話畫面處理的一部分。

## OO 概念與設計重點

此標頭體現了**單一職責原則（SRP）**：把「對話選項的副作用套用」從 `GameController` 的 `HandleDialog` 中抽出，形成一個有明確邊界、可獨立測試的函式。同時也展現了**開放/封閉原則（OCP）**的思路：新增選項種類或副作用型別時，只需修改此函式，而不需修改 `GameController` 的主流程。

冪等保護邏輯是一種「防禦性程式設計」模式：在副作用的套用點而非在 UI 層防護重複觸發，使防護不依賴選單是否重現，更為健壯。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/DialogChoiceApply.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/DialogChoiceApply.h) · [← 全檔索引](../files-index.md)
