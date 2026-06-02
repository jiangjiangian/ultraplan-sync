---
id: "file:src/engine/platform/Harness.cpp"
type: source
path: src/engine/platform/Harness.cpp
domain: engine
bucket: platform
loc: 334
classes: [HarnessState]
sources: ["src/engine/platform/Harness.cpp"]
---
# `Harness.cpp`

> **一句定位**：測試 / 錄製 harness 的完整實作：以 `HarnessState` pImpl 隱藏狀態，提供 `BeginFrame` / `EndFrame` 的 JSON 序列化、截圖、腳本計畫驅動與退出判定。

## 職責

`Harness` 是讓整個遊戲以決定性方式重放的框架核心。它以 `HarnessState`（pImpl 模式）隱藏所有可變狀態：腳本指標 `ScriptInput*`、截圖目錄 `shotsDir`、狀態輸出檔 `stateOut`、事件暫存 `events`、幀計數 `frame`、最大幀數 `maxFrames`、以及前一幀 World 的非擁有指標 `lastWorld`。

`MaybeAttach()` 是工廠函式：讀取環境變數 `UMBRELLA_SCRIPT`；若為空則直接回傳非啟用的 `Harness`（一切不變）；否則設 `active=true`、載入腳本、讀取 `UMBRELLA_SHOTS/UMBRELLA_STATE/UMBRELLA_SPRITE/UMBRELLA_SHOT_EVERY/UMBRELLA_MAXFRAMES`，設定 `Input::SetSource(script.get())` 與 `Time::SetFixedStep(1.0f/60.0f)` 使輸入與時間完全決定性。

`BeginFrame()` 在讀取輸入前呼叫：`script->Advance()`（推進傳統指令）、`++frame`、`script->ResolvePlan(lastWorld)`（以上一幀快照驅動高階動詞）。

`EndFrame(world)` 在每幀最後：儲存 `lastWorld = &world`（唯讀快照）；若達截圖間隔則呼叫 `::TakeScreenshot` 並移動至目標目錄（繞過 raylib 強制寫入 CWD 的問題）；若 stateOut 開啟則呼叫 `DumpStateJson` 序列化幀狀態；清空 `events`；若達 `maxFrames` 則設 `quitReq=true`。

`ShouldQuit()` 三重條件：`quitReq`、`script->WantsQuit()`、計畫驅動完成（`frame >= 1 && HasPlan() && PlanDone()`）。

`DumpStateJson` 是最複雜的部分，序列化：`frame`、`dt`、`semester`、`player`（位置 / karma / money / rain / umbrella / cursedTaint（非零才輸出）/ flags / consumables）、`dialog`（active / npcId / atChoice / cursor / line / choices）、`building`、`invOpen`、`top_hud` / `bottom_hud`（已過期的輸出空字串）、`objects`（active 數量 + NPC id 陣列）、`events` 陣列。

`WireEvents()` 安裝五種 EventType 的 sink lambda，將事件暫存進 `st.events` 供 `EndFrame` 序列化。

`KnownFlags()` 靜態函式列出所有要追蹤的任務旗標名稱（約 30 個），用於篩選 Player 的旗標輸出。

## 關鍵內容（類別 / 函式 / 資料）

- `HarnessState` struct — pImpl：`active`、`ScriptInput*`、`shotsDir/statePath/spritePath`、`shotEvery/maxFrames/frame`、`quitReq`、`stateOut`、`events`、`lastWorld`。
- `KnownFlags()` — 靜態；列出約 30 個任務旗標名稱。
- `EscapeJson(string_view)` — JSON 字串轉義（`"` `\` `\n` `\r` `\t` + 控制字元 `\uXXXX`）。
- `EventName(EventType)` — EventType enum 轉字串。
- `DumpStateJson(HarnessState&, World&)` — 完整幀狀態序列化為 JSON 一行。
- `Harness::MaybeAttach()` — 工廠；讀環境變數決定是否啟用 harness。
- `Harness::WireEvents()` — 安裝 5 種事件的收集 lambda。
- `Harness::BeginFrame()` — `Advance` + `++frame` + `ResolvePlan`。
- `Harness::EndFrame(world)` — 截圖 + 序列化 + 清事件 + maxFrames 守門。
- `Harness::ShouldQuit()` — 三重退出判定。

## 相依與在架構中的位置
- **#include（往外）**：`Harness.h`、`ScriptInput.h`（計畫驅動）、`Flags.h`（任務旗標名稱）、`World.h`（EndFrame 唯讀快照）、`Player.h`（pos/karma/money 等）、`DialogState.h`（對話序列化）、`SemesterStateMachine.h`（章節名稱）、`GameObject.h`（Objects 走訪）、`EventBus.h`（WireEvents）、`Input.h`（SetSource）、`Time.h`（SetFixedStep）
- **被誰使用（往內）**：—（葉節點；由 `main.cpp` 的 `MaybeAttach()` 建立，`SceneManager::Run` 呼叫 BeginFrame/EndFrame）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine / platform 層；`BeginFrame` 在輸入讀取前、`EndFrame` 在 Draw 後，包覆每幀的完整觀察窗口；屬於 [Harness](../concepts/arch-harness.md) 架構

## OO 概念與設計重點

`Harness` 是 [Harness](../concepts/arch-harness.md) 架構的核心：pImpl（`HarnessState` 作為 `unique_ptr` 成員）隱藏複雜實作，對外介面極簡（Active / BeginFrame / EndFrame / ShouldQuit / WireEvents）。設計嚴格遵守 MVC 純度：`EndFrame` 只以 `const World*` 唯讀快照，絕不寫入 World；`lastWorld` 的非擁有指標在 `BeginFrame` 被「下一幀」使用，使計畫解析成為「上一幀已記錄狀態」的純函式，達到逐位元一致的重播性。`WireEvents` + `ScopedSubscribe`（或 `Subscribe` 在此取 non-scoped）體現 [Observer](../concepts/pat-observer.md)；`MaybeAttach` 是 [Strategy](../concepts/pat-strategy.md) 的輕量版（環境變數決定輸入策略）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/engine/platform/Harness.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/Harness.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [Observer](../concepts/pat-observer.md) · [Strategy](../concepts/pat-strategy.md) · [RAII](../concepts/oo-raii.md)
