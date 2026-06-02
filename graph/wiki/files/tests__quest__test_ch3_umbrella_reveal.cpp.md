---
id: file:tests/quest/test_ch3_umbrella_reveal.cpp
type: test
path: tests/quest/test_ch3_umbrella_reveal.cpp
domain: tests
bucket: quest
loc: 116
classes: []
sources: ["tests/quest/test_ch3_umbrella_reveal.cpp"]
---
# `test_ch3_umbrella_reveal.cpp`

> **一句定位**：驗證 Ch3 真傘「給線索後才現身、位於體育館左側（不被遮擋）」，而 Ch4 真傘仍在進場時就無條件生成於體育館後方——兩條路線彼此獨立。

## 職責

此測試以三個 TEST_CASE 規格化真傘的生成位置與時機改變。核心問題：Ch3 的 TrueUmbrella 原本在進場時立即生成於 (1640,375) 體育館內，造成被建築遮擋的問題。修改後的行為：

1. **Ch3 進場時不生成**，`MaybeSpawnChapter3Umbrella()` 在 `kFlagKnowsUmbrellaLoc` 設定前每幀都是 false。
2. **C 系學姊揭露後才生成一次**，且位置改到 `kChapter3UmbrellaPos`（x < 1493，體育館左緣之西，可見）。
3. **Ch4 維持不變**：進場立即生成在 (1640, 375)，作為 Ending A 的彩蛋備援路線。
4. **非 Ch3 時 `MaybeSpawnChapter3Umbrella` 一律無操作**，即使旗標已設。

## 關鍵內容（類別 / 函式 / 資料）

- `CountTrueUmbrellas(World&)`：計算 World 中 `TrueUmbrella` 的數量（局部輔助函式）。
- `TEST_CASE("Ch3 TrueUmbrella 延後到 Flag_KnowsUmbrellaLoc 後才生成，離開章節隨名冊清除")`：Ch3 進場後 CountTrueUmbrellas==0；無旗標時 MaybeSpawn 返回 false；設旗標後返回 true（數量 0→1）；單次（再呼叫仍 1）；生成位置 x < 1493 且等於 `kChapter3UmbrellaPos`；離開章節後歸零。
- `TEST_CASE("Ch4 TrueUmbrella 仍在進場時生成於體育館後方（維持不變）")`：Ch4 進場後 Count==1；位置精確為 (1640, 375)；`MaybeSpawnChapter3Umbrella` 在 Ch4 仍為 false。
- `TEST_CASE("MaybeSpawnChapter3Umbrella 在非 Ch3 時為無操作")`：Ch1（預設）和 Ch2 均不生成，即使旗標已設。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`，`game/world/World.h`，`game/entities/Player.h`，`game/entities/TrueUmbrella.h`，`game/quest/Chapter3Quest.h`（`kChapter3UmbrellaPos`），`engine/events/EventBus.h`，`game/state/SemesterState.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：測試 Spawn 步驟（`MaybeSpawnChapter3Umbrella` 在 GameController 每幀被呼叫）與章節轉場的清除行為。

## OO 概念與設計重點

延後生成（與 Ch1 `MaybeSpawnChapter1VictimUmbrella` 同構）的測試確保「給線索才現身」的設計意圖不被未來的修改破壞。「Ch4 維持不變」的獨立測試則確保修改 Ch3 邏輯時不意外影響 Ch4 的彩蛋路線。`x < 1493`（體育館左緣）的幾何斷言把視覺可見性與程式邏輯直接掛鉤。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch3_umbrella_reveal.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch3_umbrella_reveal.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
