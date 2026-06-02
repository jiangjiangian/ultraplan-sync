---
id: file:src/game/world/BuildingTracker.cpp
type: source
path: src/game/world/BuildingTracker.cpp
domain: game
bucket: world
loc: 29
classes: []
sources: ["src/game/world/BuildingTracker.cpp"]
---
# `BuildingTracker.cpp`

> **一句定位**：每幀更新玩家所在建築並在「所在建築改變」的那一幀發布 `EnteredBuilding` 事件（單邊觸發，非逐幀重複）。

## 職責

此檔屬於 game / world 層，只實作 `BuildingTracker::Update(Vec2 playerCenter)` 一個函式。它以玩家中心座標呼叫 `detail::NearestContaining(playerCenter, buildings::kAll)` 取得當前所在建築（若不在任何建築內則為 null），再與上一幀記錄的 `current_` 比較：若所在建築發生變化，才更新 `current_` 並透過 `nccu::events::Sink().Publish(Event{EventType::EnteredBuilding, found->name})` 發布進入事件。若 `found` 為 null（離開所有建築）則不發布事件，僅更新 `current_`。

此「差異偵測 + 單邊觸發」設計確保 `EnteredBuilding` 事件每次進入新建築只發布一次，而非每幀連續發布，避免重複觸發章節邏輯或 UI 提示。

## 關鍵內容（類別 / 函式 / 資料）

- `BuildingTracker::Update(Vec2 playerCenter)` — 唯一成員函式；查詢所在建築，差異比較後選擇性發布 `EnteredBuilding` 事件，回傳當前建築指標（供 `World` 快取以回答 `CurrentBuildingName()`）。
- `current_` — 成員指標，記錄上一幀已知的所在建築（`nullptr` = 不在任何建築內）。

## 相依與在架構中的位置

- **#include（往外）**：`BuildingTracker.h`（類別宣告）、`EventBus.h` / `EventSink.h`（`Sink().Publish`）、`Color.h`（via 傳遞依賴）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；`World` 擁有一個 `BuildingTracker buildingTracker_` 成員，在每幀 `World::Update` 中呼叫 `Update(playerCenter)`。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層；位於每幀管線的早期（Movement 之後、Collision 前後）。`EnteredBuilding` 事件被章節 Quest 函式和 HUD `StatusPanel` 等訂閱者接收。

## OO 概念與設計重點

`BuildingTracker` 是「差異觸發器」的典型小工具類：以 `current_` 記憶上一次狀態，僅在真正轉換時才用 [Observer](../concepts/pat-observer.md)（`EventBus`）通知，避免逐幀重複的副作用。整個實作只有 16 行有效程式碼，體現 SRP（單一職責：只做建築進入偵測）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/BuildingTracker.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/BuildingTracker.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
