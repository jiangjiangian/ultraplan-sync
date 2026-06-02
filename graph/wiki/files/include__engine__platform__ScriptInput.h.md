---
id: file:include/engine/platform/ScriptInput.h
type: header
path: include/engine/platform/ScriptInput.h
domain: engine
bucket: platform
loc: 131
classes: [ScriptInput, Directive, Step]
sources: ["include/engine/platform/ScriptInput.h"]
---
# `ScriptInput.h`

> **一句定位**：可決定的腳本化 InputSource，支援傳統計時指令和高階計畫動詞（goto / interact / choose / advance / wait），使遊戲可無頭且逐位元可重現地驅動。

## 職責

`ScriptInput final : public nccu::engine::input::InputSource` 是 `Harness` 安裝的腳本化輸入來源，替換 `LiveInput`（透過 `Input::SetSource`）。

**兩種語法可交錯**，在同一腳本檔中自由混用：
1. **傳統計時指令**（行首為幀號整數）：`<frame> down <KEY>` / `<frame> up <KEY>` / `<frame> press <KEY>` / `<frame> quit`。邊緣語意比照 raylib：`IsPressed` 僅在按下幀為 true，`IsReleased` 僅在放開幀為 true，其間每幀 `IsDown` 為 true。`press` 指令自動在下一幀放開。
2. **高階計畫動詞**（行首為動詞字串）：`goto <X> <Y>`（軸向移動走向世界座標）、`interact <npcId>`（定位 NPC + goto + 按 E 直到對話開啟）、`choose <index>`（在選單中選取指定選項）、`advance`（輕點對話前進鍵）、`wait <frames>`（可決定間隔）、`quit`（完成後結束）。

**可決定性保證**：計畫每幀由 `ResolvePlan(World*)` 根據「當前步驟 + 上一幀 `EndFrame` 擷取的 World 快照」的純函式決定合成的按鍵邊緣，不依賴牆鐘時間、無亂數。相同腳本 + 相同模擬 → 相同 `state.jsonl`（逐位元一致）。

**狀態管理**：傳統路徑以 `byFrame_`（`unordered_map<int, vector<Directive>>`）儲存計時指令；共用按鍵集合 `down_ / pressed_ / released_` 由傳統路徑和計畫路徑共同寫入，使兩種來源的按鍵對 `GameController / Player` 無從區別。計畫以 `plan_`（`vector<Step>`）+ `planPc_`（程式計數器）+ `planSub_`（子階段）+ `planWatchdog_`（有界進度看門狗）管理。

`Advance()` 每幀在讀輸入前呼叫（步進傳統時間軸一幀）；`ResolvePlan(World*)` 在其後呼叫（解析當前計畫步驟，注入合成按鍵）。`WantsQuit()` 讓 `Harness` 知道腳本請求結束；`HasPlan()` / `PlanDone()` 讓計畫驅動的執行在最後一個動詞完成後自動結束。

## 關鍵內容（類別 / 函式 / 資料）

- **`ScriptInput`**（`final : InputSource`）：
  - `Load(istream&)`：從串流解析（傳統 + 計畫）。
  - `LoadFile(const string&)`：從檔案載入。
  - `Advance()`：步進時間軸一幀，應用 press 的自動放開。
  - `ResolvePlan(const World* world)`：依 World 快照解析計畫步驟，注入合成按鍵邊緣。
  - `WantsQuit() const noexcept → bool`、`HasPlan() const noexcept → bool`、`PlanDone() const noexcept → bool`。
  - `IsDown / IsPressed / IsReleased(Key) const noexcept override`：查詢當前按鍵集合。
- **`Directive`**（私有 struct）：`Kind`（`Down / Up / Press / Quit`）+ `key`（int）——傳統時間軸的一行指令。
- **`Step`**（私有 struct）：`Verb`（enum `Goto / Interact / Choose / Advance / Wait / Quit`）+ `x, y`（float）+ `arg`（string）+ `n`（int）——計畫動詞的一個步驟。
- **`SynthDown / SynthUp / SynthPress(int key)`**（私有方法）：把單一合成按鍵邊緣套用到 `down_ / pressed_ / released_` 集合。
- **傳統狀態**：`byFrame_`（`unordered_map<int, vector<Directive>>`）、`autoUp_`（`vector<int>`）、`frame_`（`int`）。
- **共用按鍵狀態**：`down_ / pressed_ / released_`（`unordered_set<int>`）、`quit_`（`bool`）。
- **計畫狀態**：`plan_`（`vector<Step>`）、`planPc_`（size_t）、`planSub_`（int）、`planWatchdog_`（int）。

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/input/Input.h`（繼承 `InputSource`）；標準庫 `<istream>`, `<unordered_map>`, `<unordered_set>`, `<vector>` 等；以前向宣告引入 `nccu::World`（計畫解析需唯讀存取）。
- **被誰使用（往內）**：`src/engine/platform/Harness.cpp`（建構並安裝）、`src/engine/platform/ScriptInput.cpp`（實作）、`src/engine/platform/ScriptResolver.cpp`（解析器輔助）；多個測試（`test_scriptinput / test_scriptinput_classic_move / test_scriptinput_plan / test_ch1_spine_reachable / test_i6_interact_reach`）。
- **繼承 / 實作 / 體現**：繼承並實作 `InputSource`；realizes [決定性 autoplay（Harness）](../concepts/arch-harness.md)。
- **每幀管線 / MVC 角色**：engine/platform 層；在 `BeginFrame` 中被呼叫（先 `Advance()` 後 `ResolvePlan()`），於 `GameController` 讀取輸入前就緒。

## OO 概念與設計重點

`ScriptInput` 是 [Strategy 模式](../concepts/pat-strategy.md) 的具體策略：替換 `LiveInput` 成為 `Input::Source()`，讓整個輸入查詢路徑無縫改向腳本，無需修改任何呼叫端。

高階計畫動詞（`goto / interact / choose`）是比原始鍵碼更高層次的「意圖抽象」——每個動詞被編譯成一系列合成按鍵邊緣，讓腳本作者以語意而非逐幀鍵碼描述行為，大幅降低腳本維護成本。`planWatchdog_` 確保每個動詞有有界進度（防止無窮等待），是可決定性的安全護欄。

[arch-harness](../concepts/arch-harness.md) 可決定性的核心：`ResolvePlan` 為 World 快照的純函式，相同輸入永遠產生相同合成邊緣，使 `state.jsonl` 可作為回歸測試的確定性基準。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/ScriptInput.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/ScriptInput.h) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [Harness](../concepts/arch-harness.md)
