---
id: "file:src/game/controller/screens/DialogScreen.cpp"
type: source
path: src/game/controller/screens/DialogScreen.cpp
domain: game
bucket: controller
loc: 171
classes: []
sources: ["src/game/controller/screens/DialogScreen.cpp"]
---
# `DialogScreen.cpp`

> **一句定位**：對話框凍結畫面的完整輸入處理：游標移動、按住 E 自動推進、攤販購買確認、章節 / 結局閘門路由、旗標鎖定，以及名冊同幀結算。

## 職責

`HandleDialog(EventBus& bus, World& world, Vendor*& pendingVendor, InputHandler& input, SceneRouter& sceneRouter)` 是最複雜的螢幕處理器（171 行），在 `world.Dialog().Active()` 為真時凍結其他遊戲邏輯並回傳 `true`：

**pendingVendor 清理**：若攤販對話已不在開啟中（`NpcId != kVendorContext`），清 `pendingVendor=nullptr` 防懸空指標。

**游標移動**：`AtChoice()` 為真時，Up/Down 呼叫 `dlg.MoveChoice`。

**E 鍵推進**（`input.TickDialogAdvance(dt)` 回傳 `advanceE`）：
- 若 `c = dlg.Advance()` 回傳非空（有確認的選項）：
  - **攤販分支**（`npc == kVendorContext && pendingVendor && atChoice`）：最後索引（= stockSize）為放棄，直接 Close + SettleRoster；其餘索引呼叫 `pendingVendor->TryBuy(stockIdx)` 後 CheckChapterGates + SettleRoster。購買後刻意不 CheckEndingGates（待結局自白關閉後才解算，防突兀切換）。
  - **泛用選項**：`ApplyDialogChoice(*p, *c)`（業力 + 旗標）。
    - `suit_senior` 確認（非退出選項）→ `SetFlag(kFlagSuitSeniorChoiceMade)`（互斥漣漪旗標鎖）。
    - 第四章 `ta` 確認 → `SetFlag(kFlagTaFinaleChoiceMade)`；呼叫 `TryGrantTaFinaleUmbrella`（體諒路線歸還傘 → Flag_HasTrueUmbrella）。
    - `TryBuyAuntieUglyUmbrella`（第一章福利社阿姨 (c) 的醜傘購買，npc + 標籤雙重守門，冪等）。
    - `CheckEndingGates` → `CheckChapterGates`（結局先於章節）。

**非活躍對話**：呼叫 `input.ResetDialogAdvance()` 清除按住計時，確保下一段對話從頭開始。

**每個確認分支結尾**均呼叫 `sceneRouter.SettleRoster(world)` 確保同幀名冊更新（fsm 轉場 → View 當幀就反映）。

## 關鍵內容（類別 / 函式 / 資料）

- `HandleDialog(EventBus&, World&, Vendor*&, InputHandler&, SceneRouter&) -> bool` — 171 行；完整攤販/NPC/系統選項的確認路由。
- 攤販分支：`npc == kVendorContext && pendingVendor && atChoice`；`stockIdx >= stockN` 放棄；否則 `TryBuy`。
- `kDialogExitLabel` 判斷：「我再想想…」退出不設任何旗標，不 CheckEndingGates（防過早觸發結局）。
- `TryGrantTaFinaleUmbrella` — 體諒路線給傘（`Flag_ConsoledTA` 確認後呼叫）。
- `TryBuyAuntieUglyUmbrella` — 第一章阿姨醜傘購買（冪等，npc + 標籤雙守門）。
- `sceneRouter.SettleRoster(world)` — 每個確認分支結尾呼叫，確保名冊同幀更新。

## 相依與在架構中的位置
- **#include（往外）**：`DialogScreen.h`；`DialogChoiceApply.h`（`ApplyDialogChoice`）；`InputHandler.h`；`SceneRouter.h`；`VendorMenu.h`（`kVendorContext`）；`World.h`、`Player.h`、`DialogState.h`、`DialogOpener.h`（`kDialogExitLabel`）；`ChapterGate.h`；`Chapter1Quest.h`（`TryBuyAuntieUglyUmbrella`）；`Chapter4Quest.h`（`TryGrantTaFinaleUmbrella`）；`Flags.h`；`EndingGate.h`；`SemesterStateMachine.h`、`SemesterState.h`；`Vendor.h`；`Input.h`、`Key.h`、`Time.h`
- **被誰使用（往內）**：—（由 `GameController::HandleDialog()` 薄轉發呼叫）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：game / controller 層；在 GameController::Update 的對話凍結分支中執行，為 Survival→…→Sweep 管線的「凍結提前返回」優先級第三（EndingMenu > PauseMenu > Dialog > Inventory）

## OO 概念與設計重點

`HandleDialog` 是從 `GameController` SRP 抽出的「對話螢幕」自由函式，集中所有對話確認的業務路由。延後-解算結局設計（攤販確認後不 CheckEndingGates，待自白關閉後才解算）體現「先讀敘事節拍、再觸發狀態切換」的玩法設計原則，對應 [State](../concepts/pat-state.md) 模式的轉場守門。「退出選項（`kDialogExitLabel`）不設旗標」是精確的邊界條件設計，防止「想想就跑觸發結局 C / B」的 bug。`SettleRoster` 的多點呼叫（每個確認分支尾端）是「FSM 轉場 → View 同幀反映」不變式的實施點。`pendingVendor*` 的非擁有指標在此函式的每個出口被清空或使用，是 RAII 不持有但嚴格管理所有權的實踐。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/controller/screens/DialogScreen.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/controller/screens/DialogScreen.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md) · [Observer](../concepts/pat-observer.md) · [MVC](../concepts/arch-mvc.md) · [Command](../concepts/pat-command.md)
