---
id: file:include/game/quest/ChapterPickups.h
type: header
path: include/game/quest/ChapterPickups.h
domain: game
bucket: quest
loc: 71
classes: [PickupPlacement]
sources: ["include/game/quest/ChapterPickups.h"]
---
# `ChapterPickups.h`

> **一句定位**：各章節散落零錢拾取點的資料配置——迴圈經濟「探索」收益來源，反死鎖底線的資料層實現。

## 職責

`ChapterPickups.h` 定義了各章節地圖上零錢拾取點的位置與面額，供 `WorldSpawn` 在章節進入時生成 `CashPickup` 物件。這是「玩家探索地圖 → 撿散落零錢 → 籌措市集花費」迴圈經濟中「探索」收益來源的完整規格。

`ChapterPickups(SemesterState)` 函式以靜態局部向量儲存各章節配置，回傳常量引用（零複製）。

**Ch1 配置**（五個，共 50 元）：分布於指南路/中央校園，刻意避開五個原型 NPC 生成點、四把傘（y~1280）與申請書 pickup（560,1725），確保「探索」的空間與「任務物件」的空間不相干擾。

**Ch2 配置**（三個，共 40 元）：總量刻意「大於」圖書館地下室自販機飲料售價 35 元（`EnergyDrink::kPrice` 透過 `kNpcCh2Vendor`），沿撿筆記路線分布，使身無分文抵達的玩家仍能湊出喚醒學霸的道具錢。標頭注解明確說明「chapter2.md 承諾的反死鎖底線」，是設計意圖與程式碼的直接對應。

**Ch3/Ch4**：目前為空向量（尚未配置）。`Interlude` 也回傳空（市集的收益來源是攤位而非硬幣）。

`PickupPlacement` struct 只有兩個欄位：`pos`（世界座標）和 `value`（金額）。

## 關鍵內容（類別 / 函式 / 資料）

- **`struct PickupPlacement`**（`namespace nccu`）：`pos`（`Vec2`，拾取點世界座標）、`value`（`int`，金額）。
- **`inline const std::vector<PickupPlacement>& ChapterPickups(SemesterState state)`**：取指定章節的拾取點配置；靜態局部向量，零複製回傳。
- `kChapter1`：五個拾取點（10+10+20+5+5 = 50 元，Ch1 地圖分布）。
- `kChapter2`：三個拾取點（10+10+20 = 40 元，Ch2 地圖分布，反死鎖底線）。
- `kChapter3`、`kChapter4`：空（尚未配置）。
- `kNone`：Interlude 等狀態回傳。

## 相依與在架構中的位置

- **#include（往外）**：`include/game/state/SemesterState.h`（章節狀態型別）、`include/engine/math/Vec2.h`（拾取點座標型別）；`<vector>`。
- **被誰使用（往內）**：`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（生成零錢拾取物）；`tests/quest/test_economy_loop.cpp`（驗證迴圈經濟可行性）、`tests/quest/test_spawn_reachability.cpp`（驗證拾取點可達性）。
- **繼承 / 實作 / 體現**：—（純資料 struct + 工具函式）。
- **每幀管線 / MVC 角色**：Model 初始化資料（非每幀）。`WorldSpawn::SpawnChapterPickups` 進入章節時呼叫，生成 `CashPickup` 物件；未被拾取的硬幣在下次狀態切換時由 `Sweep` 清除（每次造訪章節只有一輪）。

## OO 概念與設計重點

`ChapterPickups` 是「資料驅動配置」的典型：以靜態局部 `vector` 儲存各章節配置，`switch` 分派讓呼叫端以章節狀態為鍵取配置，而不需知道存在哪個具體向量。靜態局部（而非全局靜態）避免了初始化順序問題，且回傳常量引用確保零複製開銷。

Ch2 配置的「40 > 35」設計是「機制支持敘事承諾」的精確實現：chapter2.md 承諾的「反死鎖底線」不只是敘事設計，在資料層有具體數字保障。這讓測試（`test_economy_loop.cpp`）能驗證此不變式不被意外破壞。空的 Ch3/Ch4 配置是「明確聲明」的待辦（而非隱式遺忘），通過 `switch` 的完整性讓編譯器能感知缺少的 case。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ChapterPickups.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterPickups.h) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
