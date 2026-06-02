---
id: file:include/game/state/ChapterToast.h
type: header
path: include/game/state/ChapterToast.h
domain: game
bucket: state
loc: 89
classes: []
sources: ["include/game/state/ChapterToast.h"]
---
# `ChapterToast.h`

> **一句定位**：章節/幕間/結局轉移的過場 HUD 提示文字工具，以 inline 函式封裝文字對映與 `EventBus` 發布，同時提供幕間出口提示的單次播放閂鎖輔助。

## 職責

`ChapterToast.h` 提供三個 inline 函式與兩個 `constexpr` 字串常數，處理學期狀態機轉移的視覺回饋與幕間出口的觸覺提示。

`ChapterTransitionToast(SemesterState target)` 是純文字對映，把目標狀態轉為對應繁中提示字串（「✓ 進入第一章 加退選」至「✓ 抵達結局」），以值回傳方便呼叫端裝飾文字，所有字串控制在對話框單行字數預算內。

`PublishChapterTransitionToast(EventBus& bus, SemesterState target)` 在事件匯流排上發布過場提示，固定走 `HudSlot::Top`（頂端 HUD 槽），使同一幀在 Bottom 槽發布的其他訊息（真傘拾取、業力提示）能與章節橫幅並存不互相覆蓋。

`kInterludeArrivalHint`（市集中央引導提示）與 `kInterludeExitPrep`（南側出口離場提示）是兩個 constexpr 字串。`MaybeAnnounceInterludeExit(EventBus&, bool& latched)` 實作「每次造訪只播一次」的閂鎖邏輯：`latched` 首次為 false 時發布提示並設為 true，後續呼叫為 no-op 直到閂鎖重置（重置由呼叫端在幕間進入時負責）。這避免玩家在出口邊界來回抖動時洗版 HUD。

## 關鍵內容（類別 / 函式 / 資料）

- `ChapterTransitionToast(SemesterState target) → string`：純文字對映，列舉全部 9 個狀態的過場提示。
- `PublishChapterTransitionToast(EventBus& bus, SemesterState target)`：在 `HudSlot::Top` 發布過場提示，空字串時為 no-op。
- `kInterludeArrivalHint = "市集中央。逛完後往南離開"`：抵達市集中央的引導提示。
- `kInterludeExitPrep = "準備離開市集"`：進入南側出口觸發區的離場提示。
- `MaybeAnnounceInterludeExit(EventBus& bus, bool& latched) → bool`：單次播放閂鎖；發布時回傳 true，已閂時為 no-op 回傳 false。

## 相依與在架構中的位置

- **#include（往外）**：`EventBus.h`（`EventBus::Publish`、`Event`、`EventType::ShowMessage`、`HudSlot`）、`SemesterState.h`（轉移目標型別）
- **被誰使用（往內）**：`include/game/controller/EventWiring.h`、`src/game/controller/GameController.cpp`、`src/game/controller/SceneRouter.cpp`（章節轉移）、`src/game/quest/ChapterGate.cpp`、`src/game/state/EndingGate.cpp`（結局觸發）；測試（`test_scene_router.cpp`、`test_chapter_transitions.cpp`、`test_interlude_exit_feedback.cpp`、`test_two_hud_channels.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層輔助工具——在章節轉移（`SceneRouter`、`ChapterGate`、`EndingGate`）觸發時呼叫，透過 [Observer（EventBus）](../concepts/pat-observer.md) 送出 `ShowMessage` 事件；`MaybeAnnounceInterludeExit` 在 GameController 的幕間出口位置判定時呼叫。

## OO 概念與設計重點

`MaybeAnnounceInterludeExit` 的閂鎖模式是輕量級的**有狀態工具函式**：以傳入 `bool&` 讓呼叫端持有狀態（避免在此標頭引入靜態狀態），同時讓 GameController 整合與回歸測試走完全相同的程式路徑（測試只需提供 `false` 初始閂鎖），這是依賴注入（DIP）在閂鎖狀態上的輕量體現。`HudSlot::Top` 的固定槽使用是對「多頻道 HUD 設計」的尊重，允許同一幀多訊息並存而不互搶。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/state/ChapterToast.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/state/ChapterToast.h) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
