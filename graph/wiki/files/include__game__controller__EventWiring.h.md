---
id: "file:include/game/controller/EventWiring.h"
type: header
path: include/game/controller/EventWiring.h
domain: game
bucket: controller
loc: 158
classes: []
sources: ["include/game/controller/EventWiring.h"]
---
# `EventWiring.h`

> **一句定位**：把各 `EventBus` 訂閱者（Observer 模式的觀察者）安裝到匯流排的 inline 接線函式集合，涵蓋記錄、章節轉場、HUD 橫幅鏡射和業力提示四條管線。

## 職責

`EventWiring.h` 是 game controller 層的「Observer 安裝工廠」——所有與 `EventBus` 訂閱相關的業務邏輯都集中在此，而非散落在 `GameController` 建構子內。標頭全部以 inline 自由函式定義，無對應的 `.cpp`，呼叫端直接 `#include` 後使用。

四個核心接線函式各自負責一條訂閱管線：

1. **`WireLoggingSubscribers`**：訂閱 `ShowMessage` 和 `UmbrellaClaimed`，把文字印到 `cout`。純 I/O，不持有遊戲狀態。

2. **`WireStateTransitionSubscribers`**：訂閱 `EnteredBuilding`（更新 HUD 當前建築標籤）和 `UmbrellaClaimed`（章節清關）。包含兩個並列的 `if` 分支——Ch1 取得 `TrueUmbrella` 時進入市集過場（設 `returnTo=Chapter2_Midterms`）；Ch3 同構邏輯（設 `returnTo=Chapter4_Finals`）。轉場觸發後呼叫 `PublishChapterTransitionToast` 發布橫幅提示。所有 lambda 以參考捕捉 `semester`、`currentBuildingName`、`bus`，生命週期由呼叫端（`GameController`）保證。

3. **`WireHudMessageSubscriber`**：訂閱 `ShowMessage`，把事件文字鏡射進 `World::SetHudMessage(e.slot, e.text)`，供 View 繪製淡出提示。事件的 `slot` 欄位把文字導向 HUD 的 Top 或 Bottom 通道，使章節轉場提示與一般敘事提示可並存同幀不覆蓋。

4. **`WireKarmaToastSubscriber`**：訂閱 `KarmaChanged`，把帶正負號的業力差值（如 `"+5"`）重發為 `ShowMessage`（加「業力 」前綴）。捕捉的 `bus` 與來源相同，避免用 `EventBus::Instance()` 單例。過濾空字串與 `"+0"`/`"-0"` 以保持 HUD 乾淨。

**`WireDefaultSubscribers`** 是彙整函式，保留原本的單一進入點讓 `main.cpp` 既有呼叫不需改動。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `WireLoggingSubscribers(EventBus& bus)` | 訂閱 `ShowMessage`/`UmbrellaClaimed` → `cout` 輸出。 |
| `WireStateTransitionSubscribers(bus, semester, currentBuildingName)` | Ch1/Ch3 傘取得章節轉場 gate；`EnteredBuilding` HUD 標籤更新。 |
| `WireHudMessageSubscriber(bus, world)` | `ShowMessage` → `World::SetHudMessage`（Top/Bottom slot 感知）。 |
| `WireKarmaToastSubscriber(bus)` | `KarmaChanged` → 「業力 ±N」`ShowMessage` 重發；過濾零差值。 |
| `WireDefaultSubscribers(bus, semester, currentBuildingName)` | 彙整前兩個函式的便利包裝。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/events/EventBus.h`（匯流排與訂閱 API）、`include/game/state/ChapterToast.h`（`PublishChapterTransitionToast`）、`include/game/state/SemesterStateMachine.h`（`SemesterStateMachine`、`SemesterState`）、`include/game/world/World.h`（`World::SetHudMessage`）。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（建構子呼叫 `WireDefaultSubscribers` 等）；多個測試（`test_scene_router`, `test_ch1_quest`, `test_ch3_quest`, `test_chapter_gate`, `test_karma_toast`, `test_two_hud_channels` 等）直接呼叫個別接線函式以隔離驗證訂閱行為。
- **繼承 / 實作 / 體現**：—（純自由函式標頭）
- **每幀管線 / MVC 角色**：Controller 層的「EventBus 安裝」工具；在 `GameController` 建構時執行，不參與每幀管線本身。訂閱者的執行時機在事件發布點（任意幀）。

## OO 概念與設計重點

`EventWiring.h` 是 [Observer 模式（pat-observer）](../concepts/pat-observer.md) 的安裝層：它把觀察者（lambda 訂閱者）與被觀察者（`EventBus` 上的事件型別）「接線」，使 `GameController` 本身不直接持有每個訂閱的業務邏輯。把接線拆成四個細粒度函式符合 **SRP**：每個函式只負責一條訂閱管線，測試可個別驗證。

`WireKarmaToastSubscriber` 的「捕捉 `bus` 而非取單例」的慣例是一個值得注意的架構決策：它確保測試環境中訂閱者與被測 `bus` 是同一個實例，避免測試間干擾——這是 [EventBus Singleton（pat-singleton）](../concepts/pat-singleton.md) 與依賴注入並存時的最佳實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/EventWiring.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/EventWiring.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
