---
id: file:include/game/entities/CursedUmbrella.h
type: header
path: include/game/entities/CursedUmbrella.h
domain: game
bucket: entities
loc: 35
classes: [CursedUmbrella]
sources: ["include/game/entities/CursedUmbrella.h"]
---
# `CursedUmbrella.h`

> **一句定位**：雨傘 Template Method 樹的「詛咒傘」葉類別，`BeClaimed` 覆寫遞增道德污點計數（`cursedTaint_`）並設立 Ending B 路徑旗標。

## 職責

`CursedUmbrella` 是 `TransparentUmbrella` 繼承樹中通往 Ending B 的具體葉類別，以深沉不祥的紫色（95,45,115,255）搭配 `UmbrellaStyle::Drooping`（下垂傘面 + 黑色手柄）給出「這是錯的」的視覺印象。

覆寫 `BeClaimed(player)` 的設計刻意不採一次性 -30 業力，而是呼叫 `player->IncCursedTaint()` 遞增 `cursedTaint_` 計數器。衰減邏輯在每章過場（Ch2/Ch3/Ch4 進場）由 `ApplyCursedTaintDecay` 執行：每次扣 `5 × cursedTaint_` 業力。如此讓「順手牽羊」的道德代價橫跨整場累積——首次拾取每章扣 5，兩次拾取每章扣 10，形成滑動式道德懲罰，而非一次性重擊。`Flag_TookCursedUmbrella` 旗標仍會被設立，供 `CheckEndingGates` 的 Ending B 判定讀取。

本類別宣告為 `final`，無子類別。

## 關鍵內容（類別 / 函式 / 資料）

- **`CursedUmbrella(position)`**：建構子，硬編碼紫色 + `Drooping` 樣式，名稱為 `"CursedUmbrella"`，轉交 `TransparentUmbrella`。
- **`void BeClaimed(Player* player) override`**：認領邏輯定義於 `.cpp`；遞增 `cursedTaint_`、設 `Flag_TookCursedUmbrella`，並設立持傘狀態（`SetHeldUmbrella(HeldUmbrella::Cursed)`）。
- 繼承自 `TransparentUmbrella` 的 `Render()`（繪製 `CursedPurple` 字符）與 `Interact`／`OnPickup`（任務閘控認領流程）均不在本類別中覆寫。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/entities/TransparentUmbrella.h`（唯一直接相依）。
- **被誰使用（往內）**：`src/game/controller/GameObjectFactory.cpp`（`ObjectType::CursedUmbrella` 的工廠分支）；各 test 檔（`test_cursed_taint.cpp`、`test_umbrella_render.cpp`、`test_ch1_quest.cpp` 等）。
- **繼承 / 實作 / 體現**：繼承自 `TransparentUmbrella`（Template Method 葉類別）。
- **每幀管線 / MVC 角色**：Model 層 Item。地圖上由 `WorldSpawn` 生成；幀末 Sweep 在 `BeClaimed` 設 `isActive_=false` 後移除；每章過場業力衰減由 `SceneRouter` 呼叫 `ApplyCursedTaintDecay`。

## OO 概念與設計重點

`CursedUmbrella` 是雨傘 [Template Method](../concepts/pat-template.md) 樹的四個葉類別之一（True／Fragile／ProfessorTrap／Cursed）。它只需覆寫一個虛擬函式 `BeClaimed`——拾取流程（任務閘、`isActive_` 設定、Render 字符）全由 `TransparentUmbrella` 定稿。

`cursedTaint_` 累積機制體現了「滑動道德代價」設計：業力懲罰不是離散的單次事件，而是跨章節的長尾效應。此設計使玩家在最初拾取時可能低估後果，在後續章節中逐漸感受。這是敘事驅動的遊戲設計決策，在程式碼層面以 `Player::IncCursedTaint()` + `ApplyCursedTaintDecay()` 協作實現，職責分離清晰。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/entities/CursedUmbrella.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/entities/CursedUmbrella.h) · [← 全檔索引](../files-index.md) · 相關概念：[Template Method](../concepts/pat-template.md)
