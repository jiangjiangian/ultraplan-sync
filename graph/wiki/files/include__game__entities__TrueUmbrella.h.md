---
id: file:include/game/entities/TrueUmbrella.h
type: header
path: include/game/entities/TrueUmbrella.h
domain: game
bucket: entities
loc: 31
classes: [TrueUmbrella]
sources: ["include/game/entities/TrueUmbrella.h"]
---
# `TrueUmbrella.h`

> **一句定位**：雨傘 Template Method 樹的「真傘」葉類別，`BeClaimed` 設立通往 Ending A 的持傘條件旗標與事件。

## 職責

`TrueUmbrella` 是玩家要尋回的「正確」雨傘，以明亮天藍色（70,190,255,255）搭配完整 `UmbrellaStyle::Domed`（寬圓頂傘面）給出毫不含糊「這是乾淨／正確的傘」的視覺印象。

覆寫 `BeClaimed(player)` 設立 `SetHeldUmbrella(HeldUmbrella::True)`（即時持傘，更新背包顯示）、`SetFlag(Flag_HasTrueUmbrella)`（Ending A 的精確持傘條件）、並發出對應事件（`UmbrellaClaimed("TrueUmbrella")` 事件由 `EventBus` 驅動 Ch1→Interlude 轉場，或在 Ch3 觸發 Ch3→Interlude 轉場）。

此為四把劇情傘中唯一觸發「正確」結局路徑的葉類別，設計上是最短、最乾淨的 `BeClaimed` 實作：無額外計數器、無跨章節業力衰減，僅旗標 + 事件。

## 關鍵內容（類別 / 函式 / 資料）

- **`TrueUmbrella(position)`**：建構子，硬編碼藍色（70,190,255,255）+ `Domed` 樣式，名稱 `"TrueUmbrella"`。
- **`void BeClaimed(Player* player) override`**：設立持傘狀態 + `Flag_HasTrueUmbrella` + 發 `UmbrellaClaimed` 事件，定義於 `.cpp`。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/TransparentUmbrella.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`、`src/game/entities/TrueUmbrella.cpp`；測試：`test_factory.cpp`、`test_roles.cpp`、`test_umbrella_render.cpp`、`test_ch3_umbrella_reveal.cpp`、`test_chapter_transitions.cpp`、`test_ripple_seed_flags.cpp`。
- **繼承 / 實作 / 體現**：繼承自 `TransparentUmbrella`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層 Item，Ch1/Ch3 地圖上由 `WorldSpawn` 生成；`BeClaimed` 後 `isActive_=false`，幀末 Sweep 移除；`UmbrellaClaimed` 事件觸發 `EventWiring` 的章節轉場。

## OO 概念與設計重點

`TrueUmbrella` 是 [Template Method](../concepts/pat-template.md) 的「最小葉類別」體現：僅覆寫一個函式，其餘全部繼承中間層的定稿邏輯。與 `CursedUmbrella`（污點計數）和 `ProfessorTrapUmbrella`（追兵計數）相比，它的 `BeClaimed` 最為精簡，符合「真傘是乾淨的選擇」的敘事設計。

[Observer（EventBus）](../concepts/pat-observer.md) 的使用讓 `TrueUmbrella` 不需知道 `SemesterStateMachine`：它只發出事件，訂閱者（`EventWiring`）負責決定觸發何種轉場。這是架構 DIP（依賴反轉）的體現——葉類別不向上依賴章節狀態。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/TrueUmbrella.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/TrueUmbrella.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md) · [Observer](../concepts/pat-observer.md)
