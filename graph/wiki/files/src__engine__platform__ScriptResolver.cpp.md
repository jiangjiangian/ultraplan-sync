---
id: "file:src/engine/platform/ScriptResolver.cpp"
type: source
path: src/engine/platform/ScriptResolver.cpp
domain: engine
bucket: platform
loc: 247
classes: []
sources: ["src/engine/platform/ScriptResolver.cpp"]
---
# `ScriptResolver.cpp`

> **一句定位**：`ScriptInput::ResolvePlan` 的完整實作：將高階計畫動詞（`goto` / `interact` / `choose` / `advance` / `wait` / `quit`）編譯為每幀合成按鍵邊緣，以前一幀 World 快照為輸入，具決定性。

## 職責

此檔案是 `ScriptInput.cpp` 拆出的第二個 Translation Unit，專責 `ScriptInput::ResolvePlan(const World*)`——高階計畫解析器。設計上，此函式以前一幀 `World` 快照（`lastWorld`，由 `Harness::EndFrame` 儲存）為唯讀輸入，不改寫 World，是「已記錄模擬的純函式」，確保重播逐位元一致。

五個移動常數（`kPxPerFrame=3.0`、`kArriveEps=2.0`、`kGotoBudget=4000`、`kInteractBudget=4200`、`kChooseBudget=64`、`kAdvanceBudget=8`）均由玩法推導（180 px/s ÷ 60 fps = 3 px/frame），以幀預算而非時鐘防止軟鎖。

`AxisKeyToward(px, py, tx, ty)` — 貪心軸向移動：先收斂 X，再收斂 Y，回傳對應的 D/A/S/W 鍵碼，到達後回傳 -1。

`FindNpc(World&, string_view)` — 線性掃描存活物件，找 `NpcId() == id` 的第一個。

`ResolvePlan` 的六個 verb 分支：
- **Quit**：設 `quit_=true`，結束計畫。
- **Wait**：放開移動鍵，計數 `planSub_` 達 `step.n` 幀後完成。
- **Goto**：以 `AxisKeyToward` 計算按鍵，`SynthDown` 持續移動；達 `kGotoBudget` 結束（有界）。
- **Interact**（NPC 形式）：走向 NPC 原點（碰撞體頂住）；以鏡像 `GameController` 的 `kInteractReach`（LargeTargets 模式下 16 px，否則 8 px）膨脹 AABB 探測盒，盒重疊時 `SynthPress(E)`；對話開啟即完成。
- **Interact**（座標形式）：走向 `(step.x, step.y)` 並每幀按 E（在 pHit 與目標 24x24 盒相交時）；對話開啟則提前完成；走到達則完成（拾取物不打開對話）。
- **Choose**：逐行按 E 翻頁直到 `AtChoice()`；移動游標到目標索引；按 E 確認。
- **Advance**：單次 `SynthPress(E)` 推進一行/頁後完成（設計文件：「一次 E 點按就是一步」）。

純傳統腳本（`plan_.empty()`）在 `ReleaseMoveKeys` 之前就返回，確保絕不干擾傳統 `WASD` 指令的按住狀態。

## 關鍵內容（類別 / 函式 / 資料）

- `kPxPerFrame=3.0f`、`kArriveEps=2.0f` — 到達判定常數（由玩法速度推導）。
- `kGotoBudget=4000`、`kInteractBudget=4200`、`kChooseBudget=64`、`kAdvanceBudget=8` — 有界幀預算防軟鎖。
- `AxisKeyToward(px,py,tx,ty)` — 貪心軸向 → raylib 鍵碼（先 X 後 Y，-1 表示已到達）。
- `FindNpc(World&, string_view)` — 線性掃描找 NPC。
- `ScriptInput::ResolvePlan(const World*)` — 計畫機（planPc_ / planSub_ / planWatchdog_）；六個 verb 分支；唯讀 World；以 `SynthDown/SynthUp/SynthPress` 輸出按鍵。

## 相依與在架構中的位置
- **#include（往外）**：`ScriptInput.h`（成員宣告）；`World.h`、`Player.h`、`GameObject.h`、`DialogState.h`（計畫讀取 World 狀態）；`Key.h`（`ToRaylibKey`）；raylib
- **被誰使用（往內）**：—（由 `Harness::BeginFrame` 呼叫 `script->ResolvePlan`；本 TU 只含 `ResolvePlan` 定義，與 `ScriptInput.cpp` 共同構成 `ScriptInput` 類別）
- **繼承 / 實作 / 體現**：—（`ResolvePlan` 是 `ScriptInput` 的成員函式）
- **每幀管線 / MVC 角色**：engine / platform 層；在 `BeginFrame` 內、讀取輸入前執行；屬於 [Harness](../concepts/arch-harness.md) 架構

## OO 概念與設計重點

`ResolvePlan` 是 [Strategy](../concepts/pat-strategy.md) 的計畫執行引擎：高階語意（「走到 NPC」「選選項 2」）被轉譯為每幀合成按鍵，使遊戲邏輯完全不感知腳本存在。「鏡像 GameController 的互動幾何」（`kInteractReach`、`pHit.Intersects`、`CheckCollision`）是防止 harness 軟鎖的關鍵設計：腳本的「現在可按 E」閘與引擎的「這次按下會生效」閘必須完全一致，確保決定性行為。有界幀預算（`kGotoBudget` 等）是防軟鎖的防禦性設計，以幀計數而非時鐘確保重播一致。純傳統腳本的提前返回（`if (plan_.empty()) return`）是向後相容的邊界條件守護。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/platform/ScriptResolver.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/ScriptResolver.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [Harness](../concepts/arch-harness.md)
