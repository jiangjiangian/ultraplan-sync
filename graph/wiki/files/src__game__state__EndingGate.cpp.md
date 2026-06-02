---
id: file:src/game/state/EndingGate.cpp
type: source
path: src/game/state/EndingGate.cpp
domain: game
bucket: state
loc: 96
classes: []
sources: ["src/game/state/EndingGate.cpp"]
---
# `EndingGate.cpp`

> **一句定位**：四種結局的優先序判定函式，是每幀在第四章非對話幀輪詢並觸發結局狀態轉移的唯一閘門。

## 職責

此檔屬於 game / state 層，包含單一公開函式 `CheckEndingGates`，由 `GameController` 在每個非對話幀呼叫，決定是否將學期狀態機轉移至某個結局 `Ending_*` 狀態。

判定有兩道前置守衛：（1）`semester.Current() != Chapter4_Finals` 時直接返回（結局判定只在第四章生效）；（2）`dialog.Active()` 時延後返回，確保決定性旗標設立的同一幀若還有後續台詞未關閉，必須等玩家讀完再觸發，否則 `Transition() + Close()` 會吞掉那一幀的台詞。由於旗標皆持久，延後只推遲、絕不漏判。

四結局的優先序為 **A → B → D → C**：
- **結局 A**：業力 > 80 AND `kFlagHasTrueUmbrella`（第四章重新取得）AND `kFlagConsoledTA`（對助教體諒）。
- **結局 B**：`kFlagTookCursedUmbrella` OR 業力 < 0 OR「冷淡終局」（`kFlagTaFinaleChoiceMade && !kFlagConsoledTA`）。冷淡終局分支是防止第四章卡死的補充：否則選了終局選單卻未達 A，所有閘門皆落空，遊戲永久無出路。
- **結局 D**：`kFlagConsoledTA`（體諒）且未達 A（前面已排除 A），也非 B。業力落在 [0,80] 的苦甜結局。
- **結局 C**：`kFlagBoughtUglyUmbrella` OR `kFlagTaFinaleChoiceMade`（安全的最終預設，確保四章樹必有一支觸發）。

每個觸發分支都呼叫 `semester.Transition(Ending_*)` + `PublishChapterTransitionToast` + `dialog.Close()` 後立即返回，確保單次輪詢只觸發一個結局。

## 關鍵內容（類別 / 函式 / 資料）

- `CheckEndingGates(EventBus&, Player&, SemesterStateMachine&, DialogState&)` — 唯一的公開函式；無副作用地讀取旗標，在條件成立時觸發狀態轉移與事件。
- `kFlagHasTrueUmbrella`、`kFlagConsoledTA`、`kFlagTookCursedUmbrella`、`kFlagTaFinaleChoiceMade`、`kFlagBoughtUglyUmbrella` — 判定條件所依賴的五個關鍵旗標常數（來自 `Flags.h`）。
- `PublishChapterTransitionToast(bus, state)` — 每個結局觸發時發布過場提示事件（來自 `ChapterToast.h`）。
- `coldFinale` — 區域 bool：`kFlagTaFinaleChoiceMade && !kFlagConsoledTA`，閘控「冷淡終局→結局 B」這一防卡死路徑。

## 相依與在架構中的位置

- **#include（往外）**：`EndingGate.h`（函式宣告）、`Flags.h`（`kFlag*` 常數）、`ChapterToast.h`（`PublishChapterTransitionToast`）、`EventBus.h`（發布事件）、`Player.h`（讀業力 / 旗標）、`SemesterStateMachine.h`（呼叫 `Transition`）、`SemesterState.h`（狀態 enum）、`DialogState.h`（`Active()` 守衛）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；由 `GameController::Update` 在每個非對話幀呼叫。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Controller 層；位於每幀管線末段（Spawn 之後、Sweep 之前的結局判定步驟）。修改 `SemesterStateMachine` 狀態屬 Model 寫入，透過 `EventBus` 通知 Observer。

## OO 概念與設計重點

此函式是四結局的「哨兵記錄」設計的實作核心：結局不是 `IChapterState` 的子類別，而是透過 [State](../concepts/pat-state.md) 機器的 `Transition` 加 `inEnding_` 旗標表示（`SemesterStateMachine` 的設計）。`CheckEndingGates` 本身作為純函式（無類別成員），以「優先序分支 + 立即 return」取代複雜的條件嵌套，讓四條路徑互斥且每條都短小可讀。`dialog.Active()` 的延後機制是防禦性設計的典型案例：用持久旗標保證判定絕不遺漏，同時給玩家空間看完台詞。[Observer](../concepts/pat-observer.md) 模式（`EventBus::Publish`）用於通知 UI 顯示過場提示，維持 Model 不直接感知 View 的架構紅線。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/state/EndingGate.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/state/EndingGate.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md) · [MVC](../concepts/arch-mvc.md)
