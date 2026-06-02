---
id: file:tests/harness/test_scriptinput_plan.cpp
type: test
path: tests/harness/test_scriptinput_plan.cpp
domain: tests
bucket: harness
loc: 235
classes: [Frame]
sources: ["tests/harness/test_scriptinput_plan.cpp"]
---
# `test_scriptinput_plan.cpp`

> **一句定位**：驗證 `ScriptInput` 高階計畫動詞（`goto / interact / choose / advance / wait`）的正確性與確定性重播——同一腳本兩次執行的狀態軌跡必須逐位元相同。

## 職責

此測試是 `ScriptInput` 計畫動詞系統的核心規格，使用與 harness 相同的執行順序（每幀：`Advance()` → `ResolvePlan(snapshot)` → `controller.Update()` → 擷取快照）驅動完整的 `GameController`。

四個 TEST_CASE 覆蓋：

1. **`goto` 純函式性**：以 3 px/格移動，目標落在抵達誤差 < 3 px 內。
2. **`interact victim` 確定性**：透過繞行路線（因南牆缺口限制，無法直線到達）抵達苦主，最終位置落在 8 px 觸及帶內。
3. **`goto` + `interact victimumb`（非擋路 Item）端到端操作**：先設定 `kFlagSuitSeniorChoiceMade` 觸發延遲生成，再走到 (1700,1610) 的苦主雨傘撿取物按 E，斷言 `kFlagHasVictimUmbrella` 被設——證明合成 E edge 等效於手動腳本化輸入。
4. **確定性重播**：同一混用腳本（goto + interact + advance + goto）跑兩次，比對 `vector<Frame>` 逐元素相等。

## 關鍵內容（類別 / 函式 / 資料）

- `Frame`：儲存每格可觀察狀態的 struct：`(x, y, dialog, cursor, npc)`；兩次執行須逐元素相等。
- `RunPlan(script, maxFrames)`：輔助函式，建立全新 World + GameController，跑完腳本後回傳 `vector<Frame>` 軌跡；包含 RAII 式 teardown（`SetSource(nullptr)` 等）。
- `FindNpc(world, id)`：在 World 的 Objects 中線性搜尋指定 npcId 的活躍物件。
- `kRouteToVictimStaging`：經地形與 NPC 驗證、穿過南牆 x≈1041 缺口的繞行路線字串，通往苦主 (1660,1010) 正南方暫存點。
- `TEST_CASE("plan：`goto` 在 epsilon 內把玩家驅動到淨空目標")`：路線 `(500,1860)→(1000,1860)→(1000,1300)`；最終位置在 1 格誤差內。
- `TEST_CASE("plan：`interact victim` 確定性地抵達該 NPC")`：繞行路線後 interact，斷言 Y 軸落在觸及帶 [23, 32]。
- `TEST_CASE("plan：goto+E 在可達的（非擋路）item 上實際操作遊戲")`：端到端演練 `interact victimumb` → `kFlagHasVictimUmbrella`。
- `TEST_CASE("plan：replay 具確定性——兩次執行逐位元相同")`：`a == b`（向量逐元素）。
- `TEST_CASE("plan：classic 計時指令與動詞穿插時仍可解析")`：混用 `goto` 和 `0 down D`、`2 quit`，逐格驗證 D 按住狀態。

## 相依與在架構中的位置

- **#include（往外）**：`engine/platform/ScriptInput.h`，`game/controller/GameController.h`，`engine/events/EventBus.h`，`game/world/World.h`，`game/entities/Player.h`，`game/dialog/DialogState.h`，`game/dialog/DialogSource.h`，`engine/core/GameObject.h`，`engine/input/Input.h`，`engine/platform/Time.h`，`game/quest/Flags.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：直接驗整個 harness 管線（每幀的 Advance→ResolvePlan→Update→Snapshot 序列）。

## OO 概念與設計重點

確定性重播保證是 [harness 架構](../concepts/arch-harness.md) 的核心屬性：解析器是 (計畫步驟, World 快照) 的純函式（無 wall-clock、無 RNG），因此同一腳本兩次執行的軌跡逐位元相同，等同於兩份相同的記錄檔。這使自動化測試可靠且可重現，也使 CI 可信賴。

延遲生成（`kFlagSuitSeniorChoiceMade` → `MaybeSpawnChapter1VictimUmbrella`）的前置設定展示了測試如何準確模擬真正的遊戲進度約束，而非繞過它們。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/harness/test_scriptinput_plan.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/harness/test_scriptinput_plan.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
