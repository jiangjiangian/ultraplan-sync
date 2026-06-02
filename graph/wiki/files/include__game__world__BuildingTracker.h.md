---
id: file:include/game/world/BuildingTracker.h
type: header
path: include/game/world/BuildingTracker.h
domain: game
bucket: world
loc: 72
classes: [BuildingTracker]
sources: ["include/game/world/BuildingTracker.h"]
---
# `BuildingTracker.h`

> **一句定位**：偵測玩家踏入建築觸發矩形的「單邊轉換」，並發布 `EnteredBuilding` 事件；World 持有一個此物件追蹤當前建築。

## 職責

本檔提供兩個緊密協作的元件：模板函式 `detail::NearestContaining` 負責在有重疊觸發矩形的建築清單中以「最近中心」消歧，以及類別 `BuildingTracker` 負責在玩家第一幀踏入某建築時發布事件、後續停留幀靜默。

`NearestContaining` 以範圍模板參數設計，使測試可傳入合成的建築清單而無需依賴由 Tiled 產生的 `buildings::kAll`，實現了 **消歧邏輯** 與 **實際布局資料** 的解耦。距離完全相等時以 `b.name < found->name`（字典序）打破平手，確保結果具有確定性。

`BuildingTracker::Update()` 維護一個 `current_` 指標（無建築時為 `nullptr`），每幀比較新的 `NearestContaining` 結果與舊值：若發生轉換（進入新建築）則透過 `EventBus` 發布 `EventType::EnteredBuilding`。走進空地會清除 `current_` 但不發事件。

## 關鍵內容（類別 / 函式 / 資料）

- `detail::NearestContaining<Range>(p, range) -> const buildings::Building*`：在給定範圍中，找到觸發矩形包含點 `p` 的最近中心建築；距離相等以名稱字典序決勝；模板參數使單元測試可注入任意建築集合。
- `BuildingTracker`（class）：持有 `current_` 指標，唯一的公開方法：
  - `Update(playerCenter) -> const buildings::Building*`：每幀呼叫一次，必要時發布進入事件，回傳當前所在建築。
  - `Current() const noexcept -> const buildings::Building*`：讀取目前所在建築（不在任何建築內為 `nullptr`）。
- 私有成員 `current_{nullptr}`：上一幀所在的建築指標，用於偵測單邊轉換。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/Buildings.h`（`buildings::Building` 資料型別與 `kAll` 表）；`engine/math/Vec2.h`（`playerCenter` 的座標型別）；`<limits>`（`std::numeric_limits<float>::max()` 初始化距離）。
- **被誰使用（往內）**：`include/game/world/World.h`（World 作為成員持有 `BuildingTracker tracker_`）；`src/game/world/BuildingTracker.cpp`（Update 的實作）；`tests/world/test_building_tracker.cpp`（單元測試）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：屬於 Model 層（`game/world`），`Update()` 於 GameController 的每幀管線中被呼叫，偵測建築進入後透過 EventBus 觸發事件，最終影響章節狀態與 HUD 顯示。

## OO 概念與設計重點

`NearestContaining` 以 **泛型範圍模板** 抽象輸入資料，讓同一份邏輯既能在正式遊戲使用 `buildings::kAll`，又能在測試中接受合成陣列，是一種輕量的 **策略注入**。`BuildingTracker` 本身是樸素的 **狀態機**（只有「在建築中」與「在空地」兩個狀態），以最小狀態（一個指標）追蹤邊緣轉換。整體維持 **Model 層的純粹性**：`Update()` 的實作透過 `EventBus` 發事件，而非直接修改 View 或呼叫渲染函式，符合 [MVC](../concepts/arch-mvc.md) 架構的相依方向。

## 連結

[🕸 圖譜節點](../../index.html#node=file:include/game/world/BuildingTracker.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/world/BuildingTracker.h) · [← 全檔索引](../files-index.md) · 相關概念：[MVC](../concepts/arch-mvc.md) · [Observer](../concepts/pat-observer.md)
