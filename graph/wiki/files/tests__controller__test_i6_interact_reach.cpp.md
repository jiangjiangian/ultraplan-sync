---
id: "file:tests/controller/test_i6_interact_reach.cpp"
type: test
path: tests/controller/test_i6_interact_reach.cpp
domain: tests
bucket: controller
loc: 164
classes: [Outcome]
sources: ["tests/controller/test_i6_interact_reach.cpp"]
---
# `test_i6_interact_reach.cpp`

> **一句定位**：驗證 harness 的 `interact <id>` 計畫動詞能以正確的充氣 AABB 觸及幾何，在玩家被 NPC 碰撞箱貼齊擋住時仍成功按 E 開啟對話，並確認執行結果的完全確定性。

## 職責

本檔使用 `ScriptInput`（harness 的腳本化輸入源）驅動 `GameController`，重現 harness 本身的 `BeginFrame/ResolvePlan/Update/EndFrame` 執行模型。包含兩個 `TEST_CASE`：

**核心鎖定點**：`interact victim` 動詞必須對 Ch1 苦主（已移到綜合院館 `{1660, 1010}`）成功開啟對話。苦主是 `BlocksMovement()` NPC，物理會讓玩家恰好貼齊（不嚴格重疊）；動詞實作必須以充氣後的 `kInteractReach` AABB 判定何時按 E，而非以移動箱本身（否則貼齊後永遠不重疊，對話永不開啟）。由於苦主不在原始生成列上、且南側有校園牆，`kRouteToVictimStaging` 提供一條 23 個 `goto` 指令的繞行路線，把玩家導到 `x=1660` 的淨空列，讓最後的 `interact victim` 在 Y 軸上做貼齊靠近。驗證：`dialogOpened == true`、`npc == "victim"`、`endY >= vy`（未穿越）且 `|endY - vy| <= 32`（在觸及範圍內）、`|endX - vx| <= 2`（Y 軸列對齊）。

**確定性**：同一組 script 跑兩次（`RunInteract("victim", 3000)` 兩次），所有欄位逐位元相同：`dialogOpened`、`npc`、`openedAtFrame`、`endX`、`endY`。這是 harness「純函式」保證：`(計畫步驟, World 快照)` 決定一切，無隱藏狀態。

`RunInteract` helper 構造 `World + GameController + ScriptInput`，執行迴圈與 harness 完全相同：`in.Advance()` → `in.ResolvePlan(snapshot)` → `controller.Update()` → `snapshot = &world`，直到 `in.WantsQuit()` 或 `in.PlanDone()`。

`Outcome` struct 記錄 `dialogOpened`、`npc`、`openedAtFrame`、`endX/endY`，讓確定性案例可逐欄比對。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Outcome`：每次執行的觀察值記錄（對話是否開啟、NPC id、開啟格、最終位置）。
- `kRouteToVictimStaging`：23 個 `goto` 指令組成的繞行路線字串，讓玩家繞過南側校園牆到達苦主正南方。
- `RunInteract(npcId, maxFrames, preRoute)`：組裝 `World+GameController+ScriptInput`，執行 harness 模型迴圈，回傳 `Outcome`。
- `FindNpc(world, id)`：在 `Objects()` 中尋找 NPC。
- `TEST_CASE("harness 的 `interact victim` 能開啟 NPC 對話（貼齊擋住時）")`：充氣 AABB 的核心鎖定點。
- `TEST_CASE("`interact` 確定性地開啟對話（兩次執行完全相同）")`：純函式確定性保證。

## 相依與在架構中的位置
- **#include（往外）**：`ScriptInput.h`、`GameController.h`、`World.h`、`Player.h`、`DialogState.h`、`DialogSource.h`、`EventBus.h`、`GameObject.h`、`Input.h`、`Time.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—（使用 `ScriptInput` 而非自定義 stub）
- **每幀管線 / MVC 角色**：端到端驗證 Movement→Collision→Interact 管線，特別是充氣 AABB 觸及範圍的幾何計算

## OO 概念與設計重點

本檔是 [arch-harness](../concepts/arch-harness.md) 的自我驗證測試：確認 harness 的 `interact` 動詞與真實遊玩路徑（`kInteractReach` 充氣 AABB）的幾何相符，且 bit-for-bit 確定性。確定性 case 是回歸測試的黃金標準，任何引入非確定性的改動（如加入 `rand()`、系統時鐘或 GL 狀態）都會讓這個 case 失敗。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_i6_interact_reach.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_i6_interact_reach.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[arch-harness](../concepts/arch-harness.md)
