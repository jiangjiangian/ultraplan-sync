---
id: file:tests/quest/test_spawn_reachability.cpp
type: test
path: tests/quest/test_spawn_reachability.cpp
domain: tests
bucket: quest
loc: 165
classes: [Spot]
sources: ["tests/quest/test_spawn_reachability.cpp"]
---
# `test_spawn_reachability.cpp`

> **一句定位**：載入實際出貨的碰撞地形遮罩，以洪水填充（BFS）從玩家出生點驗證所有任務關鍵生成點都不嵌牆且可達；無資產時優雅跳過。

## 職責

此測試檔是地圖完整性的「縱深防禦」測試，防止以下兩類靜默錯誤：（1）生成點落在實心地形（物件嵌牆）；（2）生成點被地形封死（玩家到不了）。

測試流程：
1. 從 `nccu::LoadTerrainMask()` 載入碰撞遮罩；若為空（`mask.Empty()`），代表 `resources/` 目錄未追蹤，優雅跳過（MESSAGE + return，不 FAIL）。
2. 呼叫 `GameplaySpots()` 收集所有生成點：玩家/四把傘/申請書（硬寫座標）+ `DefaultNpcSpawns()` + `AmbientStudentSpawns()` + 各章 NPC 名冊 + Ch2 筆記 + Ch1 金幣 + 幕間/Ch2/Ch4 攤商 + Ch4 體育館後 TrueUmbrella + Ch3 TrueUmbrella（`kChapter3UmbrellaPos`）。
3. **不嵌牆**：對每個 Spot 呼叫 `mask.BlockedBox(x, y, 24, 24)` 斷言 false。
4. **洪水填充可達性**：以 8px 網格從玩家出生點 (500, 1860) BFS，建立 `seen[]` 陣列；對每個生成點在 ±3 格範圍內找到 `seen` 標記即為可達。

## 關鍵內容（類別 / 函式 / 資料）

- `struct Spot { const char* name; float x; float y; }`：生成點記錄（匿名 namespace）。
- `GameplaySpots()`：收集所有生成點的 helper，動態呼叫所有 `ChapterNpcSpawns`、`ChapterQuestItems`、`ChapterPickups`、`ChapterVendors`。
- `kBox = 24.0f`：玩家碰撞盒大小（對應 `world::kPlayerWidth/Height`）。
- `TEST_CASE("每個遊戲生成點都可通行且從玩家出生點可達")`：唯一 TEST_CASE，包含不嵌牆斷言 + BFS + 可達性斷言。
- `nccu::LoadTerrainMask()`：載入出貨地形遮罩。
- `nccu::CollisionMask::BlockedBox(x, y, w, h)`：盒形碰撞查詢。
- `nccu::kChapter3UmbrellaPos`：Ch3 TrueUmbrella 的生成座標常數。

## 相依與在架構中的位置

- **#include（往外）**：`CollisionMask.h`、`NpcSpawns.h`（`DefaultNpcSpawns`、`AmbientStudentSpawns`）、`ChapterSpawns.h`、`ChapterQuestItems.h`、`ChapterPickups.h`、`ChapterVendors.h`、`Chapter3Quest.h`（`kChapter3UmbrellaPos`）、`SemesterState.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 整合測試，同時涉及 Model（CollisionMask、所有生成表）。

## OO 概念與設計重點

「優雅降級」是此測試最重要的設計特點：全新 checkout 沒有地形資產時不應讓 CI 失敗，因此以 `if (mask.Empty()) { MESSAGE(...); return; }` 有條件跳過。BFS 使用 8px 網格（而非像素精度）是效能與精度的平衡；±3 格的可達性容差讓生成點不需精確落在可通行格上，只需「附近可達」即可。`GameplaySpots()` 動態呼叫所有生成表函式，確保測試與生產代碼的生成點定義同步（零重複）。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_spawn_reachability.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_spawn_reachability.cpp) · [← 全檔索引](../files-index.md)
