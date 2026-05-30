## 2. 狀態機與結局（State machine & Endings）

學期進程由 `SemesterStateMachine` 驅動。每個章節是一個 `IChapterState`（State 模式）：
切換時 `Transition()` 重建一個具體狀態物件。結局**不是** `IChapterState` 子類別——機器
以 `ending_` / `inEnding_` 哨兵記錄結局態。結局判定集中在自由函式 `CheckEndingGates()`
（`EndingGate.cpp`），每個非對話幀輪詢一次。

```mermaid
classDiagram
    direction TB
    class SemesterState {
        <<enumeration>>
        Chapter1_AddDrop
        Interlude_Market
        Chapter2_Midterms
        Chapter3_SportsDay
        Chapter4_Finals
        Ending_A
        Ending_B
        Ending_D
        Ending_C
    }
    class IChapterState {
        <<interface>>
        +Id() SemesterState*
        +Name() string_view*
        +Enter()
        +Exit()
        +Update(dt: float)
    }
    class SemesterStateMachine {
        -state: unique_ptr~IChapterState~
        -inEnding: bool
        -interludeReturnTo: SemesterState
        +Current() SemesterState
        +Transition(next: SemesterState)
        +SetInterludeReturnTo(s)
        +InterludeReturnTo() SemesterState
    }
    class Chapter1AddDrop
    class InterludeMarket
    class Chapter2Midterms
    class Chapter3SportsDay
    class Chapter4Finals
    class EndingGate {
        <<free function>>
        +CheckEndingGates(player, semester, dialog)$
    }

    IChapterState <|-- Chapter1AddDrop
    IChapterState <|-- InterludeMarket
    IChapterState <|-- Chapter2Midterms
    IChapterState <|-- Chapter3SportsDay
    IChapterState <|-- Chapter4Finals
    SemesterStateMachine o-- IChapterState : holds current
    SemesterStateMachine ..> SemesterState : reports / transitions
    EndingGate ..> SemesterStateMachine : Transition() to Ending_*
```

### 2a. 學期狀態圖（4 結局 A → B → D → C）

```mermaid
stateDiagram-v2
    [*] --> Chapter1_AddDrop : 遊戲開始 (2月加退選季)

    state Chapter1_AddDrop {
        [*] --> 綜院掉傘
        綜院掉傘 --> 對峙西裝學長 : 取得苦主請託 (Flag_PromisedVictim)
        對峙西裝學長 --> 找回傘1 : 做出選擇後現傘 + 重逢致謝
    }
    Chapter1_AddDrop --> Interlude_Market : 致謝對話關閉後推進 (LiftChapter1Clear)

    state Interlude_Market {
        [*] --> 四維道市集
        四維道市集 --> 購買道具 : 花費金幣 (Vendor TryBuy)
        四維道市集 --> 歸還管理員的傘 : 選擇性 責任感 +10
    }
    Interlude_Market --> Chapter2_Midterms : 走入觸發區（InterludeReturnTo == Ch2）

    state Chapter2_Midterms {
        [*] --> 圖書館掉傘
        圖書館掉傘 --> 喚醒學霸 : 提神飲料 (ConsumeOne)
        喚醒學霸 --> 找回傘2 : 蒐齊散落筆記 + 致謝
    }
    Chapter2_Midterms --> Interlude_Market : Flag_Ch2Cleared 對話關閉後（returnTo Ch3）
    Interlude_Market --> Chapter3_SportsDay : 走入觸發區（InterludeReturnTo == Ch3）

    state Chapter3_SportsDay {
        [*] --> 校慶掉傘
        校慶掉傘 --> 繞操場一圈 : Flag_SportsLapDone
        繞操場一圈 --> 找回傘3 : C系學姊揭示位置後現傘
    }
    Chapter3_SportsDay --> Interlude_Market : 找回傘3 / Flag_Ch3Cleared（returnTo Ch4）
    Interlude_Market --> Chapter4_Finals : 走入觸發區（InterludeReturnTo == Ch4）

    state Chapter4_Finals {
        [*] --> 終局暴雨 : 入章清空持傘 (傘再度失蹤)
        終局暴雨 --> 助教結算 : 抵達集英樓 + Flag_TaFinaleChoiceMade
        助教結算 --> 自白獨白 : TryOpenEndingConfession (對話先讀完)
    }

    note right of Chapter4_Finals
      CheckEndingGates 全部 gate 在 Chapter4_Finals，
      precedence A → B → D → C（先中先贏），且在
      dialog.Active() 時延後解析（自白／結算對話先讀完）。
      Flag_TaFinaleChoiceMade 一旦設立即為 TOTAL：四選一必中，
      不會 soft-lock。
    end note

    Chapter4_Finals --> Ending_A : karma>80 且 Flag_HasTrueUmbrella 且 Flag_ConsoledTA
    Chapter4_Finals --> Ending_B : Flag_TookCursedUmbrella ∥ karma<0 ∥ 冷酷質問結算
    Chapter4_Finals --> Ending_D : Flag_ConsoledTA 但未達 A（karma∈[0,80]）→ 風雨同行/破傘
    Chapter4_Finals --> Ending_C : Flag_BoughtUglyUmbrella ∥ 任何殘餘結算旗標（破財消災 default）

    Ending_A --> [*] : 「雨過天晴，傘還在你手上。」
    Ending_B --> [*] : 「你成為了你曾經最討厭的那種人」
    Ending_D --> [*] : 「傘破了，但你沒丟下任何人。」
    Ending_C --> [*] : 「再也不會有人拿錯你的傘了。」
```

> **幕間市集是共用的轉運站**：每一章通關後都會先回到 `Interlude_Market`，再由
> `InterludeReturnTo` 決定下一站（Ch1→Ch2、Ch2→Ch3、Ch3→Ch4）。狀態機只有一個市集
> 狀態物件，被重複進出三次；上圖刻意畫出三條「進市集 / 出市集」的邊，與
> `ChapterGate.cpp`、`EventWiring.h` 的轉場一致（亦對應 Report §3.4 的章節脊柱）。
>
> **相對舊版的改動**：新增第四個結局 **Ending_D 風雨同行**（選了「體諒」但 karma≤80 →
> `FragileBroken` 破傘）。結局判定不再只掛在對話確認當下，而是每個非對話幀輪詢
> `CheckEndingGates`；舊的「Ch1 買醜傘 → C」sibling-if 已移除，真正的 C 觸發點改為
> Ch4 集英樓便利商店的 `Vendor`（設 `Flag_BoughtUglyUmbrella`）。詳見
> `src/game/state/EndingGate.cpp`。

---

[← 回 UML 總覽](README.md) ｜ [上一節：§1 實體與道具繼承樹](1-entities.md) ｜ [下一節：§3 MVC 核心 + ISystem 模擬管線 →](3-mvc-isystem.md)
