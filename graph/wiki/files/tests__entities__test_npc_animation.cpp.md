---
id: "file:tests/entities/test_npc_animation.cpp"
type: test
path: tests/entities/test_npc_animation.cpp
domain: tests
bucket: entities
loc: 148
classes: []
sources: ["tests/entities/test_npc_animation.cpp"]
---
# `test_npc_animation.cpp`

> **一句定位**：在無 GL 環境下驗證 NPC 行走動畫格選擇（`CurrentRenderCell`）——靜止型停在 idle 格、校慶繞圈播放行走動畫、遊走 NPC 確實切換步伐欄，以及貼牆滑行時朝向列不抖動的回歸保護。

## 職責

本檔包含 5 個 `TEST_CASE`，以 `constexpr float kDt = 1.0f/60.0f` 固定步進驅動 `NPC::Update`，驗證 `CurrentRenderCell()` 的 `{col, row}` 選擇邏輯。所有斷言都在引擎層（不依賴 GL 或截圖），因此可在 headless 環境執行。

**靜止型 NPC idle 格**：`isQuestGiver=true`、`npcId="ta"` 的任務型 NPC，120 幀更新後 `col == 1`、`row == 0`（Pipoya idle 格），確認靜止型的 Update 是空操作。

**校慶繞圈跑者**：`EnableCircularRun(center, radius, angularSpeed, startAngle)` 後 240 幀，蒐集 `columnsSeen` 與 `rowsSeen`：步伐欄必須包含 0 和 2（非只停在 idle），朝向列 `>= 2` 種（繞行帶來多方向）。驗證行走動畫實際播放。

**環境遊走 NPC**：`EnableWander(speed=40, seed=12345)` 後 240 幀，至少出現欄 0 或 2（真正播放動畫而非滑行），且每幀朝向列在合法 `[0, 3]` 範圍內。

**貼牆滑行朝向回歸（最複雜 case）**：seed 51 的第一個遊走朝向為右下對角 `{1,1}`；NPC 緊貼世界底邊（`y = 2048-24`）。運動後 y 被釘住（地板），每幀淨位移為 `{d, 0}`（軸向往右）。舊 bug：`facing_` 依每幀淨位移決定（`{d,0}` → 右 row 2）與預期朝向（`{1,1}` → 下 row 0）間每幀跳動。修正後 `facing_` 綁在穩定的 `wanderDir_`（整段行程不變），故滑行期間 `movingRows.size() == 1`（只有一列）且 `movingRows.count(0) == 1`（穩定朝向下 = row 0）、`movingRows.count(2) == 0`（不是軸向位移的列）。

**暫停遊走者顯示 idle 欄**：600 幀內找到淨位移為零的幀，驗證那些幀 `col == 1`（idle 欄），確認停頓時不顯示步伐欄。

## 關鍵內容（類別 / 函式 / 資料）

- `NPC::CurrentRenderCell()` → `NPC::RenderCell {col, row}`：算繪格選擇（被測主體）。
- `NPC::EnableCircularRun(center, radius, angularSpeed, startAngle)`：校慶繞圈行為。
- `NPC::EnableWander(speed, seed)`：環境遊走行為（決定性 PRNG）。
- `NPC::Update(dt)`：推進位置與動畫狀態。
- `constexpr float kEdge = 2048.0f - 24.0f`：世界底邊座標（滑行 case 用）。

## 相依與在架構中的位置
- **#include（往外）**：`include/game/entities/NPC.h`、`include/game/gfx/WalkCycle.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：驗證 Movement 管線中 NPC 的動畫狀態更新（`IUpdatable::Update`）

## OO 概念與設計重點

貼牆滑行的回歸測試是本檔最有價值的部分：它以決定性 PRNG（seed 51）和固定地板位置，精確重現 bug 成因，並釘住修正後的不變式（`movingRows.size() == 1`）。這種「以決定性輸入重現邊界行為」的手法，避免了動畫測試中常見的「非確定性 flakiness」問題。`WalkCycle.h` 提供 Pipoya 四方向行走動畫的格座標常識，讓測試以人類可讀的「下 = row 0」表達期望。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/entities/test_npc_animation.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/entities/test_npc_animation.cpp) · [← 全檔索引](../files-index.md)
