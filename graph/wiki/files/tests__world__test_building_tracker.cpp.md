---
id: "file:tests/world/test_building_tracker.cpp"
type: test
path: tests/world/test_building_tracker.cpp
domain: tests
bucket: world
loc: 177
classes: [EventCapture]
sources: ["tests/world/test_building_tracker.cpp"]
---
# `test_building_tracker.cpp`

> **一句定位**：驗證 `BuildingTracker` 的進出建築偵測——初始為 null、進入/停留/切換發事件，以及 `NearestContaining` 的重疊消歧義算法（距離優先、平手依字典序）。

## 職責

本測試固定 `BuildingTracker` 的行為合約，覆蓋正常使用路徑與邊界算法。

**正常路徑**（使用真實 `kAll` 建築表，座標從 `CentreOf(name)` 取得）：
- 初始 `Current()` 為 null。
- 進入「操場」後 `Current()->name == "操場"`，發出一次 `EnteredBuilding` 事件（`cap.hits == 1`）。
- 停留在同棟內多幀，事件計數不增加。
- 從「操場」走到「體育館」，發出第二次事件，`cap.hits == 2`，`lastName == "體育館"`。
- 走進空地（左上角 50,50，斷言不在任何觸發矩形內）後 `Current() == nullptr`，事件計數不增加。

**NearestContaining 算法**（合成 fixture，與 `kAll` / Tiled 版面無關）：
- 矩形重疊時，距離較近的建築勝出（(140,140) 離 A 中心更近 → A 勝）。
- 相反的點選到相反的建築（(120,120) → A；(180,180) → B）。
- 完全等距平手時，UTF-8 名稱字典序較小者勝（「法學院」U+6CD5 < 「行政大樓」U+884C → 法學院勝，且不論陣列順序一致）。

`CentreOf(name)` 從 `kAll` 即時查找觸發矩形中心，使測試追蹤 Tiled 重新擺放而非寫死座標；若建築不存在回傳 (-1,-1)，以 REQUIRE 要求存在，改名時大聲失敗。

## 關鍵內容（類別 / 函式 / 資料）

- `EventCapture`：含 `hits` 計數與 `lastName` 字串，手動呼叫 `SubscribeBuilding()` 訂閱 `EnteredBuilding`。
- `CentreOf(std::string_view name)` — 輔助函式，從 `kAll` 查找建築中心座標。
- `BuildingTracker::Update(Vec2)` — 被測主要函式。
- `BuildingTracker::Current()` — 被測：回傳 `const Building*` 或 nullptr。
- `nccu::detail::NearestContaining(Vec2, array<Building, N>)` — 被測消歧義算法。

## 相依與在架構中的位置

- **#include（往外）**：`game/world/BuildingTracker.h`（受測主體）、`engine/events/EventBus.h`（事件訂閱）。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層測試，驗證 `BuildingTracker` 在 World 的 Update 管線中負責的建築邊界偵測邏輯。

## OO 概念與設計重點

算法測試與整合測試分離：正常路徑測試用真實 `kAll`（Tiled 版面若移動建築必須修改測試），算法測試用合成 fixture（僅測演算法性質，不受版面影響）。[Observer](../concepts/pat-observer.md) 事件訂閱讓行為側效果可在不直接訪問 `BuildingTracker` 內部的情況下被斷言。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/world/test_building_tracker.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/world/test_building_tracker.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Observer](../concepts/pat-observer.md)
