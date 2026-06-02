---
id: "file:src/game/controller/DialogChoiceApply.cpp"
type: source
path: src/game/controller/DialogChoiceApply.cpp
domain: game
bucket: controller
loc: 21
classes: []
sources: ["src/game/controller/DialogChoiceApply.cpp"]
---
# `DialogChoiceApply.cpp`

> **一句定位**：將確認的對話選項副作用（業力增減 + 旗標設定/清除）套用到 Player 的純函式，含一次性獎勵防重複領取守門。

## 職責

`ApplyDialogChoice(Player& player, const DialogChoice& choice)` 是一個精簡的純業務函式，套用對話選項的業力與旗標副作用：

1. **防重複領取守門**：若 `choice.setsFlag` 非空、`choice.flagValue=true`，且玩家已設有該旗標，則直接返回（一次性獎勵不重複套用）。
2. 呼叫 `player.AddKarma(choice.karmaDelta)` 加減業力。
3. 若 `setsFlag` 非空：`flagValue=true` → `SetFlag`，`flagValue=false` → `ClearFlag`。

此函式被抽出自 `GameController`（原屬 `HandleDialog` 內聯），使 `DialogScreen.cpp` 能直接取用，而不必相依整個 `GameController.h`（SRP / DIP）。測試可對此函式直接進行單元測試，不需要完整的 Controller。

## 關鍵內容（類別 / 函式 / 資料）

- `ApplyDialogChoice(Player& player, const DialogChoice& choice)` — 防重領 + `AddKarma` + `SetFlag/ClearFlag`；僅 21 行，無副作用（不發 EventBus 事件）。

## 相依與在架構中的位置
- **#include（往外）**：`DialogChoiceApply.h`；`DialogState.h`（`DialogChoice` 定義）；`Player.h`（`AddKarma` / `SetFlag` / `ClearFlag` / `HasFlag`）
- **被誰使用（往內）**：—（由 `DialogScreen.cpp` 的 `HandleDialog` 呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；在 `HandleDialog` 的「確認選項」分支中執行，屬 Controller Update 管線

## OO 概念與設計重點

「抽出 `ApplyDialogChoice`」是 SRP 拆分的典型：將「對話選項副作用」從 `GameController` 的龐大 `HandleDialog` 分離為可獨立測試的純函式。防重複領取守門（`HasFlag` 短路）確保冪等性——玩家重複對話同一 NPC 不會累積一次性獎勵（旗標 / 業力的不變式）。`choice.flagValue` 可以為 false 的設計（`ClearFlag`）支援「選項 B 撤銷先前行為」的敘事機制，比純「加旗標」更通用。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/DialogChoiceApply.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/DialogChoiceApply.cpp) · [← 全檔索引](../files-index.md)
