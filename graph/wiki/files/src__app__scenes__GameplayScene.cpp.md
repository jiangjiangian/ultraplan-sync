---
id: "file:src/app/scenes/GameplayScene.cpp"
type: source
path: src/app/scenes/GameplayScene.cpp
domain: app
bucket: scenes
loc: 116
classes: []
sources: ["src/app/scenes/GameplayScene.cpp"]
---
# `GameplayScene.cpp`

> **一句定位**：遊玩場景的實作：以角色選擇結果建構 World / View / Controller 三件組，管理事件接線，每幀推進模型，並處理 Restart / Quit 意圖。

## 職責

`GameplayScene` 是核心遊玩場景，建構子按語意順序初始化成員：`world_`（Model，`spritePath` + `loadSprites=true` + `WorldOptions`）、`view_`（View，視窗尺寸）、`controller_`（Controller，`world_` + `EventBus::Instance()`）、`audioManager_`（最後，解構時最先拆除音訊）。`SetSink(&EventBus::Instance())` 將實體層發布接縫綁到本次執行的匯流排。

建構子可選地處理 `UMBRELLA_START_STATE` 環境變數（僅限 harness 啟用時）：依字串跳轉 `SemesterStateMachine` 並呼叫 `RespawnChapterRoster`，讓截圖工具直達後段章節。

`Enter()` 呼叫 `harness_.WireEvents()`，確保錄製器的訂閱在 controller 已擁有匯流排後才安裝（解構次序正確）。

`Update()` 委託 `controller_.Update()`；讀取 `world_.PendingAppAction()`：`Restart` 時若有 `restartFactory_` 則回傳 Replace 指令（重建整條 Loading→Title→Select→Gameplay 鏈），否則 Quit；`Quit` 直接 Quit。

`Draw()` 直接呼叫 `view_.Draw(world_)` 透過 raylib 渲染。

`Exit()` 呼叫 `SetSink(nullptr)` 卸下接縫，確保重啟循環能乾淨重新綁定。

## 關鍵內容（類別 / 函式 / 資料）

- `GameplayScene(CharacterSelectResult, AudioDevice&, Harness&, int, int, RestartFactory)` — 建構 MVC 三件組；設定 EventSink；處理 `UMBRELLA_START_STATE` 跳關；設定 Player 顏色調變（`p->SetTint`）。
- `Enter()` — `harness_.WireEvents()`，確保訂閱次序。
- `Update(float)` — `controller_.Update()` + `PendingAppAction()` 路由（Restart / Quit）。
- `Draw(IRenderer&)` — `view_.Draw(world_)`。
- `Exit()` — `SetSink(nullptr)`。
- `WorldForHarnessOrNull()` — 回傳 `&world_`，使 `SceneManager` 每幀呼叫 `harness.EndFrame(world_)` 序列化狀態。

## 相依與在架構中的位置
- **#include（往外）**：`GameplayScene.h`、`EventBus.h`（Instance）、`EventSink.h`（SetSink）、`Harness.h`（WireEvents）、`DrawScope.h`（SceneManager 持有，此處知道其約定）、`WorldOptions.h`（ReadWorldOptionsFromEnv）
- **被誰使用（往內）**：—（葉節點；由 SceneBootstrap 或 restartFactory 建立）
- **繼承 / 實作 / 體現**：實作 `IScene`（Enter / Update / Draw / Exit / WorldForHarnessOrNull）
- **每幀管線 / MVC 角色**：app 層的遊玩場景；`Update` 呼叫 `GameController::Update()`，後者執行完整的 Survival→…→Sweep 管線；自身是 MVC 的組裝點

## OO 概念與設計重點

`GameplayScene` 是 [MVC](../concepts/arch-mvc.md) 三件組（World/View/Controller）的組裝點，依賴注入（DIP）——`World`、`View`、`GameController` 彼此不直接相依，由此場景連結。`SetSink` / `WireEvents` 的時序設計確保 EventBus 訂閱者的生命期與 Controller 一致，防止解構時懸空（[Observer](../concepts/pat-observer.md) 的 RAII 管理）。`UMBRELLA_START_STATE` 的跳關設計體現 [Harness](../concepts/arch-harness.md) 架構的擴充點——正常遊玩路徑完全不感知此分支。`restartFactory_` 的空值作為「錄製路徑永不重啟」的契約，而非例外（設計邊界條件）。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/GameplayScene.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/GameplayScene.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Observer](../concepts/pat-observer.md) · [Harness](../concepts/arch-harness.md) · [RAII](../concepts/oo-raii.md)
