# 《尋傘記：政大山下篇》UML 設計文件

本檔聚焦 Assignment 5 評分項目 #7「UML Class Diagram」，並附上系統的狀態機與循序圖以呈現完整動態行為。所有圖以 Mermaid 撰寫，可在 GitHub / VS Code / Typora 直接渲染。

> **圖例標記**
> - 類別後加 `[v]` 表示已在 `include/` + `src/` 內實作
> - 類別後加 `[plan]` 表示僅存於設計文件、待後續開發

---

## 1. Class Diagram（類別圖）

```mermaid
classDiagram
    direction TB

    class GameObject {
        <<Abstract>>
        #Vector2 position
        #Rectangle hitBox
        #bool isActive
        #int collisionLayer
        +Update(deltaTime: float)*
        +Draw()*
        +Interact(initiator: Player*)*
        +CheckCollision(other: Rectangle) bool
        +IsActive() bool
        +Deactivate()
    }

    class Character {
        <<Abstract>>
        #float speed
        #Vector2 direction
        #int currentFrame
        +Move(dir: Vector2, deltaTime: float)
    }

    class Item {
        <<Abstract>>
        #string itemName
        #bool isPickable
        +OnPickup(player: Player*)*
        +GetName() string
        +IsPickable() bool
    }

    class Player {
        -float rainMeter
        -int karma
        -bool hasUmbrella
        +HandleInput(deltaTime: float)
        +decreaseKarma(amount: int)
        +resetRainMeter()
        +GetKarma() int
        +HasUmbrella() bool
        +SetHasUmbrella(v: bool)
    }

    class NPC {
        -string[] dialogLines
        -int currentLine
        -bool isQuestGiver
        +Talk()
        +UpdateDialogByState(state: SemesterState)
    }

    class TransparentUmbrella {
        <<Abstract>>
        #Color umbrellaTint
        +Draw()
        +Interact(initiator: Player*)
        +OnPickup(player: Player*)
        +beClaimed(player: Player*)*
    }

    class TrueUmbrella {
        +beClaimed(player: Player*)
    }

    class FragileUmbrella {
        -float leakRate
        +beClaimed(player: Player*)
        +GetLeakRate() float
    }

    class ProfessorTrapUmbrella {
        -int spawnedEnemiesCount
        +beClaimed(player: Player*)
        +GetSpawnedEnemiesCount() int
    }

    class CursedUmbrella {
        -int karmaPenalty
        +beClaimed(player: Player*)
        +GetKarmaPenalty() int
    }

    class GameObjectFactory {
        <<Static>>
        +Create(type: ObjectType, position: Vector2)$ unique_ptr~GameObject~
    }

    class EventBus {
        <<Singleton>>
        -map~EventType, Handler[]~ handlers_
        +Instance()$ EventBus&
        +Subscribe(type: EventType, handler: Handler)
        +Publish(event: Event) const
        +Clear()
    }

    GameObject <|-- Character
    GameObject <|-- Item
    Character  <|-- Player
    Character  <|-- NPC
    Item       <|-- TransparentUmbrella
    TransparentUmbrella <|-- TrueUmbrella
    TransparentUmbrella <|-- FragileUmbrella
    TransparentUmbrella <|-- ProfessorTrapUmbrella
    TransparentUmbrella <|-- CursedUmbrella

    GameObjectFactory ..> GameObject : creates
    GameObjectFactory ..> Player : creates
    GameObjectFactory ..> TrueUmbrella : creates
    GameObjectFactory ..> FragileUmbrella : creates
    GameObjectFactory ..> ProfessorTrapUmbrella : creates
    GameObjectFactory ..> CursedUmbrella : creates

    TransparentUmbrella ..> EventBus : publishes RenderRequested
    TrueUmbrella        ..> EventBus : publishes UmbrellaClaimed / ShowMessage
    FragileUmbrella     ..> EventBus : publishes UmbrellaClaimed / ShowMessage
    ProfessorTrapUmbrella ..> EventBus : publishes UmbrellaClaimed / ShowMessage
    CursedUmbrella      ..> EventBus : publishes UmbrellaClaimed / KarmaChanged / ShowMessage
```

### 實作狀態總表

| 類別 | 狀態 | 檔案 |
|---|---|---|
| `GameObject` | [v] | `include/GameObject.h` |
| `Character` | [v] | `include/Character.h` |
| `Item` | [v] | `include/Item.h` |
| `Player` | [v] | `include/Player.h` + `src/Player.cpp` |
| `NPC` | [plan] | — |
| `TransparentUmbrella` | [v] | `include/TransparentUmbrella.h` + `src/TransparentUmbrella.cpp` |
| `TrueUmbrella` | [v] | `include/TrueUmbrella.h` + `src/TrueUmbrella.cpp` |
| `FragileUmbrella` | [v] | `include/FragileUmbrella.h` + `src/FragileUmbrella.cpp` |
| `ProfessorTrapUmbrella` | [v] | `include/ProfessorTrapUmbrella.h` + `src/ProfessorTrapUmbrella.cpp` |
| `CursedUmbrella` | [v] | `include/CursedUmbrella.h` + `src/CursedUmbrella.cpp` |
| `GameObjectFactory` | [v] | `include/GameObjectFactory.h` + `src/GameObjectFactory.cpp` |
| `EventBus` | [v] | `include/EventBus.h` + `src/EventBus.cpp` |

### GoF 設計模式對照

| 模式 | 落點 | 角色 |
|---|---|---|
| **Factory Method** | `GameObjectFactory::Create` | 由 `ObjectType` 列舉動態產生 5 種具體 GameObject |
| **Template Method** | `TransparentUmbrella::beClaimed` (純虛) | 4 個子類提供 4 種被拾取行為 |
| **Observer** | `EventBus::Subscribe` / `Publish` | UI 訂閱 `RenderRequested`、`ShowMessage`、`UmbrellaClaimed` |
| **State** | `SemesterStateMachine` (規劃中) | 學期 5 章 + 3 結局之間的轉換 |

### 架構鐵律

1. `Player` 不得 `#include` 任何具體 umbrella header — 只認 `TransparentUmbrella*`
2. `Item` / `TransparentUmbrella` 不得呼叫 `DrawText` / `DrawTexture` — 一律經 `EventBus` 廣播
3. 主迴圈不得在迭代中 `delete` GameObject — 改 `isActive_ = false`，幀末 `erase-remove` 一次掃除

---

## 2. State Diagram（學期狀態機）

```mermaid
stateDiagram-v2
    [*] --> Chapter1_AddDrop : 遊戲開始 (2月開學季)

    state Chapter1_AddDrop {
        [*] --> 綜院掉傘
        綜院掉傘 --> 集英樓尋找 : 獲得苦主線索
        集英樓尋找 --> 找回傘1 : 完成學長跑腿任務
    }

    Chapter1_AddDrop --> Interlude_Market : 找回傘1觸發時間推進

    state Interlude_Market {
        [*] --> 四維道市集
        四維道市集 --> 購買道具 : 花費金幣
        購買道具 --> 狀態重置 : 降低淋雨值累積率
    }

    Interlude_Market --> Chapter2_Midterms : 4月期中考

    state Chapter2_Midterms {
        [*] --> 圖書館掉傘
        圖書館掉傘 --> 尋找學霸 : 蒐集提神飲料
        尋找學霸 --> 找回傘2 : 喚醒學霸並交換
    }

    Chapter2_Midterms --> Chapter3_SportsDay : 找回傘2

    state Chapter3_SportsDay {
        [*] --> 運動會掉傘
        運動會掉傘 --> 攤位物物交換 : 大型市集解謎
        攤位物物交換 --> 找回傘3 : 體育館後台拯救
    }

    Chapter3_SportsDay --> Chapter4_Finals : 找回傘3

    state Chapter4_Finals {
        [*] --> 終局掉傘
        終局掉傘 --> 崩潰的校園 : 暴雨粒子特效
        崩潰的校園 --> 最終抉擇 : 抵達集英樓傘架前
    }

    Chapter4_Finals --> Ending_A_True   : karma > 80 且完整解謎
    Chapter4_Finals --> Ending_B_Bad    : karma < 0（CursedUmbrella 累積）
    Chapter4_Finals --> Ending_C_Normal : 商店購入醜綠傘

    Ending_A_True   --> [*]
    Ending_B_Bad    --> [*]
    Ending_C_Normal --> [*]
```

---

## 3. Sequence Diagram（互動循序圖）

展示玩家按下 `E` 鍵與透明傘互動時，多型動態綁定 + Observer 解耦如何協作。

```mermaid
sequenceDiagram
    participant Input as Raylib Input
    participant P as Player
    participant Map as Main Loop / MapManager
    participant U as TransparentUmbrella*
    participant C as CursedUmbrella (具體實例)
    participant Bus as EventBus
    participant UI as UI Subscriber

    Input->>P: 偵測 E 鍵 (Interact)
    P->>Map: CheckCollision(playerHitBox, 互動範圍)

    alt 找到互動目標
        Map-->>P: 回傳 GameObject* (指向 CursedUmbrella)
    else 無碰撞
        Map-->>P: nullptr（不執行）
    end

    Note over P, C: 玩家不需 downcast，只看抽象介面
    P->>U: target->Interact(this)

    Note over U, C: VTable 動態綁定到 CursedUmbrella::beClaimed
    U->>C: beClaimed(player)

    C->>P: player->decreaseKarma(50)
    C->>P: player->SetHasUmbrella(true)

    C->>Bus: Publish(UmbrellaClaimed)
    C->>Bus: Publish(KarmaChanged)
    C->>Bus: Publish(ShowMessage "你順手牽羊了！")
    Bus->>UI: 對所有 handler 廣播（snapshot 後 dispatch，避免迭代失效）
    UI-->>Bus: 訂閱者完成顯示

    C->>C: isActive_ = false (標記，不立即 delete)
    Note over Map: 主迴圈幀末 erase-remove 統一清理
```

---

## 4. 設計原則總結

| 原則 | 體現位置 |
|---|---|
| **針對介面寫程式** | `Player` 只持有 `TransparentUmbrella*`，永不 `#include` 具體傘類 |
| **單一職責** | `Item` / `Umbrella` 只管邏輯，渲染丟給 `EventBus` 訂閱者 |
| **開放封閉** | 新增「會飛的傘」只需加一個 `TransparentUmbrella` 子類 + Factory enum，主迴圈零修改 |
| **記憶體安全** | 物件以 `std::unique_ptr` 持有；移除採 `isActive_` 旗標 + 幀末 `erase-remove` |
