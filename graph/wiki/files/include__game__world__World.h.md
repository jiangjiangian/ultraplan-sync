---
id: file:include/game/world/World.h
type: header
path: include/game/world/World.h
domain: game
bucket: world
loc: 440
classes: [World]
sources: ["include/game/world/World.h"]
---
# `World.h`

> **一句定位**：MVC 的 Model 核心——擁有全部 GameObject、學期狀態機、建築追蹤器與地形遮罩，提供章節生成、幀末延遲清除與純資料的 UI 狀態，不依賴 raylib 與輸入。

## 職責

`World` 是遊戲的資料模型，嚴格禁止持有 raylib 型別或讀取輸入。View 以 `const World&` 唯讀存取、GameController 寫入它；`World` 本身不主動驅動任何渲染或輸入邏輯，是 MVC 三角的「M」角。

**物件管理**：`ObjectList`（`std::vector<std::unique_ptr<GameObject>>`）持有所有遊戲物件，front 永遠是玩家。`player_` 是對 front 物件的非擁有快取，允許 O(1) 存取而無需每幀強轉 front 元素。`Sweep()` 在幀末以單趟 `remove-erase` 移除所有被標記為非存活的物件，維持「front 即玩家」的不變式，並在玩家自身被清除前呼叫 `ClearPlayer()` 避免懸空指標。

**章節生命週期**：`RespawnChapterRoster(state)` 僅移除本次生成記錄於 `chapterRoster_` 的章節 NPC，再為新狀態生成 NPC；玩家、四把雨傘、申請書與環境學生不受影響。四個延遲生成函式（`MaybeSpawnChapter2Notes`、`MaybeSpawnChapter1VictimUmbrella`、`MaybeSpawnChapter3Umbrella`、`MaybeSpawnInterludeLibrarianReturn`）各自守衛一次性旗標，每次造訪章節僅生成一次，並在換章時由 `RespawnChapterRoster` 重置。

**純 UI 狀態**：`World` 攜帶背包開關（`inventoryOpen_`）、背包游標（`inventoryCursor_`）、暫停選單開關（`menuOpen_`）、選單游標（`menuCursor_`）、說明覆蓋層開關（`helpOpen_`）、說明頁碼（`helpPage_`）、減少動畫旗標（`reducedMotion_`）、擴大目標旗標（`largeTargets_`）等 UI 狀態。這些是純資料：無渲染、無輸入；View 負責反映、GameController 負責凍結模擬與更新。

**雙通道 HUD**：`topHudMessage_`（章節／結局重大進度提示）與 `bottomHudMessage_`（其他所有 ShowMessage）分開儲存，各自計齡，使章節清關提示與緊接其後的抵達提示能並存而不被覆蓋。

**應用層動作**：`AppAction` 列舉（None／Restart／Quit）讓暫停選單與結局選單透過 `RequestAppAction` 上報意圖，由最外層迴圈（main.cpp）安全完成 World 拆除與重建，避免 GameController 自我拆除。

## 關鍵內容（類別 / 函式 / 資料）

- **建構**：`World(playerSpritePath, loadSprites=true, opts={})` — 生成玩家與初始章節名冊，載入地形遮罩；`loadSprites=false` 供無頭單元測試略過 GPU 上傳。
- **物件存取**：`Objects()` / `GetPlayer()` / `ClearPlayer()` — 物件容器與玩家快取存取。
- **狀態機**：`Semester()` → `SemesterStateMachine&` — 學期狀態機；`Tracker()` → `BuildingTracker&` — 建築追蹤器；`Dialog()` → `DialogState&`。
- **操場繞圈**：`UpdateSportsLap()` — 累計帶號角度，繞滿一圈設立 `Flag_SportsLapDone`；`SportsLapProgress()` — 完成比例 [0,1]；`SportsLapActive()` — 是否在第三章未完成。
- **延遲生成**：`MaybeSpawnChapter2Notes()`、`MaybeSpawnChapter1VictimUmbrella()`、`MaybeSpawnChapter3Umbrella()`、`MaybeSpawnInterludeLibrarianReturn()` — 各自有一次性守衛旗標，回傳 bool 供測試判定。
- **HUD**：`SetHudMessage(slot, text)` / `TickHud(dt)` / `DismissHud(slot)` / `HudMessage(slot)` / `HudAge(slot)` / `HudExpired(slot)` — 兩通道（`HudSlot::Top`／`Bottom`）的完整 HUD 生命週期管理。
- **UI 狀態**：`InventoryOpen/SetInventoryOpen`、`InventoryCursor/SetInventoryCursor`、`MenuOpen/SetMenuOpen`、`MenuCursor/MoveMenuCursor`、`HelpOpen/SetHelpOpen`、`HelpPage/SetHelpPage`、`ReducedMotion/SetReducedMotion`、`LargeTargets/SetLargeTargets`。
- **應用動作**：`AppAction`（enum class）、`RequestAppAction`、`ClearAppAction`、`PendingAppAction`；常數 `kMenuItemCount=6`、`kEndingMenuItemCount=3`。
- **章節管理**：`RespawnChapterRoster(state)` / `Sweep()` — 換章重生與幀末清除；私有 `SpawnChapterNpcs(state)` 與 `SpawnChapterQuestItems(state)`。
- **私有資料**：`objects_`（ObjectList）、`player_`（裸指標快取）、`chapterRoster_`（章節物件原始指標向量）、`semester_`（SemesterStateMachine）、`tracker_`（BuildingTracker）、`dialog_`（DialogState）、`terrainMask_`（CollisionMask）、多個一次性守衛旗標（`ch2NotesSpawned_` 等）、操場角度累積欄位（`lapStarted_`、`lapPrevAngle_`、`lapSwept_`）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/core/GameObject.h`（物件基底）；`engine/events/HudSlot.h`（HUD 通道列舉）；`game/world/HudTiming.h`（HUD 計時常數）；`game/entities/Player.h`（快取型別）；`game/state/SemesterState.h`、`SemesterStateMachine.h`（學期狀態機）；`game/world/BuildingTracker.h`（建築追蹤器）；`game/world/CollisionMask.h`（地形遮罩）；`game/world/WorldOptions.h`（建構注入的無障礙旗標）；`game/dialog/DialogState.h`（對話狀態）。
- **被誰使用（往內）**：`include/app/scenes/GameplayScene.h`；`include/game/controller/EventWiring.h`；`src/game/controller/GameController.cpp`（主要寫入端）；`src/ui/View.cpp`（唯讀渲染端）；以及龐大的測試群。
- **繼承 / 實作 / 體現**：`realizes_concepts: MVC 核心 (arch-mvc)`。
- **每幀管線 / MVC 角色**：MVC 的 Model。GameController 在管線每階段（Survival → Movement → Collision → Spawn → Interact → Sweep）讀寫 World；View 在每幀末以 `const World&` 唯讀渲染。World 自身不主動跑任何管線邏輯。

## OO 概念與設計重點

`World` 是 [MVC](../concepts/arch-mvc.md) 模式的具體體現：嚴格的「純資料模型」——無 raylib、無輸入，View 以 `const` 唯讀、Controller 寫入。**mark-then-sweep** 的物件移除策略（`isActive_=false` 標記 + 幀末 `Sweep()` 單趟清除）避免了在迭代容器中 erase 造成的迭代器失效。建構子注入 `WorldOptions`（無障礙偏好）而非自呼叫 `std::getenv`，是 **依賴注入（DI）** 的應用，使單元測試取得確定性環境。`AppAction` 意圖向上傳遞而非 GameController 自我拆除，體現了 **命令模式（Command）** 的精神——[pat-command](../concepts/pat-command.md)。雙 HUD 通道設計是對「單通道被立即覆蓋」這一具體 bug 的明確修復，有詳盡的注解說明其「為何」。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/world/World.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/World.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Command](../concepts/pat-command.md)
