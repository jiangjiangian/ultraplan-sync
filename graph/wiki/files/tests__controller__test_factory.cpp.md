---
id: "file:tests/controller/test_factory.cpp"
type: test
path: tests/controller/test_factory.cpp
domain: tests
bucket: controller
loc: 68
classes: []
sources: ["tests/controller/test_factory.cpp"]
---
# `test_factory.cpp`

> **一句定位**：驗證 `GameObjectFactory::Create` 依 `ObjectType` 枚舉建出動態型別正確的物件，並釘住 `CursedUmbrella::BeClaimed` 的「污染值遞增、業力延遲」語意。

## 職責

本檔包含六個 `TEST_CASE`，涵蓋工廠輸出的型別正確性與 `CursedUmbrella` 的撿取語意。

前五個 case 各自以 `dynamic_cast` 驗證 `GameObjectFactory::Create` 對應的輸出型別：`Player`、`TrueUmbrella`、`FragileUmbrella`、`ProfessorTrapUmbrella`、`CursedUmbrella`，位置參數分別傳入 `{100,100}` 或 `{200,200}`。`REQUIRE(obj != nullptr)` 確保工廠不回傳 null，`CHECK(ptr != nullptr)` 確保 `dynamic_cast` 成功，完整驗證動態型別。

最後一個 case 驗證 `CursedUmbrella::BeClaimed` 的業力延遲語意：直接建立 `Player` 與 `CursedUmbrella` 並呼叫 `u.BeClaimed(&p)`，斷言：`p.GetCursedTaint() == 1`（污染值遞增）、`p.GetKarma() == before`（撿取當下 karma 不動）、`p.HasUmbrella() == true`（背包顯示詛咒傘）。業力代價延遲到後續進入新章節時由 `ApplyCursedTaintDecay` 落地（`-5 * taint`）。

## 關鍵內容（類別 / 函式 / 資料）

- `TEST_CASE("Factory 建出的 Player 動態型別正確")`：`dynamic_cast<Player*>`
- `TEST_CASE("Factory 建出的 TrueUmbrella 動態型別正確")`：`dynamic_cast<TrueUmbrella*>`
- `TEST_CASE("Factory 建出的 FragileUmbrella 動態型別正確")`：`dynamic_cast<FragileUmbrella*>`
- `TEST_CASE("Factory 建出的 ProfessorTrapUmbrella 動態型別正確")`：`dynamic_cast<ProfessorTrapUmbrella*>`
- `TEST_CASE("Factory 建出的 CursedUmbrella 動態型別正確")`：`dynamic_cast<CursedUmbrella*>`
- `TEST_CASE("CursedUmbrella BeClaimed 只增加 cursedTaint，業力延遲到 ApplyCursedTaintDecay")`：驗證 taint 遞增、karma 不動、HasUmbrella 為 true

## 相依與在架構中的位置
- **#include（往外）**：`include/game/controller/GameObjectFactory.h`（被測工廠）、`include/game/entities/Player.h`、`include/game/entities/TrueUmbrella.h`、`include/game/entities/FragileUmbrella.h`、`include/game/entities/ProfessorTrapUmbrella.h`、`include/game/entities/CursedUmbrella.h`
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—

## OO 概念與設計重點

本檔測試 [Factory Method](../concepts/pat-factory.md) 模式：`GameObjectFactory::Create(ObjectType, Vec2)` 依枚舉分派具體類別，測試以 `dynamic_cast` 驗證多型輸出。同時釘住了 [Template Method](../concepts/pat-template.md) `BeClaimed` 的延遲業力語意——詛咒傘的「道德污點永久存在」哲學透過 `cursedTaint_` 計數而非即時扣分體現。這個 case 是 `test_cursed_taint.cpp` 的入口，兩者互補。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/controller/test_factory.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/controller/test_factory.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Factory Method](../concepts/pat-factory.md) · [Template Method](../concepts/pat-template.md)
