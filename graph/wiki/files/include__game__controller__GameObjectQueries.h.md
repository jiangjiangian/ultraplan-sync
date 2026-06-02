---
id: "file:include/game/controller/GameObjectQueries.h"
type: header
path: include/game/controller/GameObjectQueries.h
domain: game
bucket: controller
loc: 43
classes: []
sources: ["include/game/controller/GameObjectQueries.h"]
---
# `GameObjectQueries.h`

> **一句定位**：走訪 `unique_ptr<GameObject>` 容器的泛型輔助函式——把「指標有效 && 仍存活（&& 可選略過某物件）」的防護收斂於一處，讓遊戲主迴圈的走訪邏輯讀起來像一行演算法組合。

## 職責

`GameObjectQueries.h` 是 game controller 層的 header-only 泛型工具標頭，定義在 `nccu::queries` 命名空間下，提供兩個 template 自由函式供遊戲主迴圈走訪物件容器使用。

核心動機：`World` 的物件容器是 `std::vector<std::unique_ptr<GameObject>>`，在走訪時需要雙重防護——「指標不為 null」且「物件仍存活（`IsActive()`）」——若不集中，這兩個檢查會散落在每個走訪迴圈中形成大量重複的 guard code。`ForEachActive` 把防護封裝為一行，呼叫端傳 lambda 描述「對每個存活物件做什麼」。

`ForEachActiveExcept` 增加了「略過特定物件」的變體，典型用途是碰撞解算時略過玩家自身：`CollisionSystem` 走訪所有動態演員的碰撞盒時，傳入 `Player*` 指標作為 `skip`，避免玩家和自身碰撞。

兩個函式皆為 template，對任何持有 `unique_ptr<GameObject>` 的容器型別（`vector`、`list` 等）和任何 `void(GameObject&)` 簽名的可呼叫物（lambda、函式指標）都適用，保持了零耦合的工具性質。

## 關鍵內容（類別 / 函式 / 資料）

| 符號 | 說明 |
|------|------|
| `namespace nccu::queries` | 所有函式的命名空間。 |
| `ForEachActive(Container& c, Fn&& fn)` | 對 `c` 中每個「非 null 且 `IsActive()`」的物件呼叫 `fn(*obj)`。 |
| `ForEachActiveExcept(Container& c, const GameObject* skip, Fn&& fn)` | 同上，但額外略過 `obj.get() == skip` 的物件；`skip` 為 `nullptr` 時不略過任何物件。 |

## 相依與在架構中的位置

- **#include（往外）**：`include/engine/core/GameObject.h`（`IsActive()` 方法與抽象基底型別）。
- **被誰使用（往內）**：`src/game/controller/GameController.cpp`（主幀迴圈走訪）、`src/game/controller/InteractDispatch.cpp`（互動候選走訪）、`src/game/controller/SimSystems.cpp`（各 ISystem 的物件走訪）、`src/ui/View.cpp`（繪製走訪）、`src/ui/world/QuestGiverIndicators.cpp`（尋找 NPC 標記）。
- **繼承 / 實作 / 體現**：—（純 template 工具）
- **每幀管線 / MVC 角色**：Controller 層共用工具；被每幀管線的所有走訪點使用（Survival → Movement → Collision → Spawn 各階段皆可使用）。View 層的走訪也使用此函式，故跨越了 MVC 邊界，但因為只讀（View 只呼叫 `Render`，不修改物件）故不違反 MVC 純度。

## OO 概念與設計重點

這兩個 template 函式是**泛型程式設計（Generic Programming）**的直接應用：以容器型別和可呼叫物作為模板參數，實現了「演算法與資料結構解耦」的目標。這也是 C++ STL 風格（`std::for_each`）在遊戲域的應用。

「指標有效 && 存活」的雙重防護被封裝為不變式，呼叫端不再需要重複撰寫，降低了 mark-then-sweep 模式（`isActive_=false` 標記、幀末 `Sweep()` 刪除）的使用摩擦：在幀中間標記失效的物件後，走訪函式自動跳過它們，無需立即刪除。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/controller/GameObjectQueries.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/controller/GameObjectQueries.h) · [← 全檔索引](../files-index.md)
