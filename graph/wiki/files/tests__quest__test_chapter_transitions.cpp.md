---
id: file:tests/quest/test_chapter_transitions.cpp
type: test
path: tests/quest/test_chapter_transitions.cpp
domain: tests
bucket: quest
loc: 385
classes: []
sources: ["tests/quest/test_chapter_transitions.cpp"]
---
# `test_chapter_transitions.cpp`

> **一句定位**：驗證每次章節/幕間/結局轉場發出的 HUD 提示字串、`TrueUmbrella` 撿取與章節清關的雙頻道 HUD 分流（Top/Bottom），以及名冊在轉場同幀更新不殘留前章 NPC 的 `SceneRouter` 契約。

## 職責

此測試檔是批次中行數最多、覆蓋面最廣的一個，整合了四類測試：

**（一）轉場字串表**：`ChapterTransitionToast` 對所有 9 個狀態（Ch1/幕間/Ch2/Ch3/Ch4/Ending_A/B/D/C）的回傳字串一對一核對，任何狀態缺口都會被捕捉。

**（二）事件→HUD 管線**：透過 `WireStateTransitionSubscribers` + `WireHudMessageSubscriber` 接線，確認 Ch1/Ch2/Ch3/Ch4 經 `UmbrellaClaimed` 或 `kFlagCh*Cleared` 轉場後，`last` 字串收到正確的章節提示；以及幕間離開時依 `returnTo` 發出目的章節提示。Ending A/B/C 路徑都確認發出「✓ 抵達結局」。

**（三）雙頻道分流回歸**（最複雜）：`TrueUmbrella::BeClaimed` 的 `UmbrellaClaimed` 事件觸發章節清關（上方槽 `HudSlot::Top`），同時傘的撿取台詞走 `HudSlot::Bottom`，兩槽互不覆蓋。測試以 `WireStateTransitionSubscribers` + `WireHudMessageSubscriber` 接上完整路徑，確認 `Top` 含「章節清關」、`Bottom` 含「TrueUmbrella」，且兩槽互不包含對方的字串。

**（四）名冊同幀更新**：`SceneRouter::SettleRoster(w)` 必須在轉場的同一幀更新 `Objects()`，不允許一幀落差。以 Ch1→Ch2 為例，轉場後 `librarian`（Ch2 專屬）立即出現。完整七段主幹測試（Ch1→…→Ending_A）每一步都確認目的名冊中的 NPC 全部出現、且無前章殘留，並驗證 `SettleRoster` 具冪等性。

## 關鍵內容（類別 / 函式 / 資料）

- `SubscribeToLatest(std::string&)` 匿名 helper：以 `ScopedSubscribe` 建立 `ShowMessage` 訂閱。
- `TEST_CASE("ChapterTransitionToast 字串表涵蓋每一個狀態")`。
- `TEST_CASE("EventWiring：Ch1 經 UmbrellaClaimed -> 幕間市集會發布提示")`。
- `TEST_CASE("ChapterGate：Ch2 -> 幕間市集會發布提示")` / Ch3 同類測試。
- `TEST_CASE("ChapterGate：幕間市集 -> returnTo 會發布目的章節提示")` + 三個 SUBCASE。
- `TEST_CASE("EndingGate：Ch4 -> Ending A/B/C 會發布「抵達結局」提示")` + 三個 SUBCASE。
- `TEST_CASE("TrueUmbrella::BeClaimed：章節提示走上方欄，撿取台詞走下方欄")`：雙頻道回歸網。
- `TEST_CASE("HudMessage 訂閱者端到端收到轉場提示")`。
- `TEST_CASE("名冊在 Transition() 同一幀跟隨 FSM 更新")`：`SceneRouter::SettleRoster` 冪等性。
- `TEST_CASE("每次轉場都關閉其 npcs[] 落差（完整主幹）")`：七段主幹遍歷 + 無殘留 NPC 驗證。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`ChapterGate.h`、`ChapterSpawns.h`、`ChapterToast.h`、`DialogState.h`、`EndingGate.h`、`EventBus.h`、`EventWiring.h`、`GameObject.h`、`HudSlot.h`、`Player.h`、`SceneRouter.h`、`SemesterStateMachine.h`、`TrueUmbrella.h`、`World.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：測試 Controller 層的事件接線（`WireStateTransitionSubscribers`）、MVC View 層的 HUD 更新（`WireHudMessageSubscriber`），以及每幀管線末端的 `SceneRouter::SettleRoster`。

## OO 概念與設計重點

雙頻道 HUD 分流是此批次最精細的測試設計：每個 `ShowMessage` 都帶 `HudSlot`（`Top` 或 `Bottom`），使章節清關提示不被同幀的撿取台詞覆蓋。`SceneRouter::SettleRoster` 的冪等性保證確保即使 `Update()` 呼叫兩次也不重複生成 NPC，對應 mark-then-sweep 的安全語義。[Observer 模式](../concepts/pat-observer.md)（EventBus）與 [State 模式](../concepts/pat-state.md)（SemesterStateMachine）在此作為被測系統的底層機制。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter_transitions.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter_transitions.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md) · [State](../concepts/pat-state.md)
