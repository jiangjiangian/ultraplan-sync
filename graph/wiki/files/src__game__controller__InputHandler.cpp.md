---
id: "file:src/game/controller/InputHandler.cpp"
type: source
path: src/game/controller/InputHandler.cpp
domain: game
bucket: controller
loc: 38
classes: []
sources: ["src/game/controller/InputHandler.cpp"]
---
# `InputHandler.cpp`

> **一句定位**：實作對話框中 E 鍵的「邊緣觸發 + 按住自動推進」邏輯，讓玩家長按 E 可快速略讀對話而不必逐次點按。

## 職責

`InputHandler::TickDialogAdvance(float dt)` 是此檔案的核心，每幀在對話框開啟期間呼叫，回傳「本幀是否應推進對話」的布林：

- **按住計時**：`Input::IsDown(Key::E)` 時累增 `eHoldMs_`（`dt * 1000`），放開時歸零（`eAutoAdvanceCooldown_` 也在放開時重置）。
- **邊緣觸發**：`edgeE = Input::IsPressed(Key::E)` 代表本幀新按下的邊緣。
- **自動觸發**：`!edgeE && IsDown(E) && eHoldMs_ >= kHoldAdvanceMs`（300 ms）時進入自動觸發：若冷卻計數 `eAutoAdvanceCooldown_ > 0` 則遞減，否則設 `autoE = true` 並重設冷卻為 `kAutoCooldownFrames`（4 幀）。
- 回傳 `edgeE || autoE`。

關鍵邊界條件：「只有 `!edgeE` 時才走自動觸發分支」確保單次按下不會在同一幀同時邊緣觸發又自動觸發，防止對話框被推進兩次。`test_input_handler` 對此約定進行了單元測試固定。`ResetDialogAdvance()` 在對話框關閉時呼叫，清空 `eHoldMs_` 和冷卻，確保下一段對話從頭開始計時。

## 關鍵內容（類別 / 函式 / 資料）

- `TickDialogAdvance(float dt) -> bool` — 按住計時 + 邊緣偵測 + 自動觸發；回傳「本幀推進」。
- `kHoldAdvanceMs`（宣告於標頭）— 按住 300 ms 後開始自動觸發。
- `kAutoCooldownFrames`（宣告於標頭）— 自動觸發冷卻幀數（4 幀），約每 4/60 ≈ 67 ms 一次。
- `eHoldMs_`、`eAutoAdvanceCooldown_`（成員）— 按住時長（ms）與剩餘冷卻計數。

## 相依與在架構中的位置
- **#include（往外）**：`InputHandler.h`（唯一直接依賴，含 `Input` / `Key` 使用）
- **被誰使用（往內）**：—（由 `DialogScreen.cpp` 的 `HandleDialog` 持有並每幀呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；在 `HandleDialog` 的對話凍結分支中，每幀於 `Advance()` 前呼叫

## OO 概念與設計重點

`InputHandler` 將「對話 E 鍵的觸發邏輯」從 `DialogScreen` 中抽出，形成可獨立測試的有狀態元件（SRP）。邊緣 / 自動分支互斥的設計防止雙重觸發，是精確的邊界條件處理——對應 `test_input_handler` 的單元測試固定。`ResetDialogAdvance()` 的顯式重置（而非依賴 `IsDown` 自然歸零）使約定明確，可讀性高。此設計也隔離了「輸入時序邏輯」，使 `DialogScreen` 本體只需詢問一個 bool，不需關心按住計時細節。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/InputHandler.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/InputHandler.cpp) · [← 全檔索引](../files-index.md)
