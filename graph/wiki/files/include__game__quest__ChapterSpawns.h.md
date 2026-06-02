---
id: file:include/game/quest/ChapterSpawns.h
type: header
path: include/game/quest/ChapterSpawns.h
domain: game
bucket: quest
loc: 156
classes: []
sources: ["include/game/quest/ChapterSpawns.h"]
---
# `ChapterSpawns.h`

> **一句定位**：以 `SemesterState` 為鍵回傳各章節的 NPC 名冊，是 `World::RespawnChapterRoster` 的純資料來源，驅動 NPC 隨章節切換。

## 職責

`ChapterSpawns.h` 定義 inline 函式 `ChapterNpcSpawns(SemesterState)` 並實作各章節的完整 NPC 配置清單，讓 World 在每次學期狀態機推進時能快速取得新章節的 NPC 陣容，而無需在 World 本身中寫死各章 NPC。

Ch1 委派給 `DefaultNpcSpawns()`（與 NpcSpawns.h 的 Ch1 字面值共享），避免重複。Ch2 對應期中考情境：圖書館管理員（isQuestGiver=true，位於中正圖書館服務台 820,545）、學霸（羅馬廣場南緣 1088,1100）、西裝學長（行政大樓正門 1220,775）、助教（中正圖書館前 900,545）、苦主（圖書館前西側角 720,560）、福利社阿姨（樂活小舖 1560,1560），全 6 個。

Ch3 對應校慶運動會：5 個劇情路人（isQuestGiver=false，坐在操場南緣 y≈910–930，刻意在跑道圓圈 r150 範圍外）加上 3 個物物交換鏈任務給予者（vendor_sausage_a / loudspeaker_b / senior_c，isQuestGiver=true，散布於羅馬廣場 980–1150,1000–1120），鏈指示器依序揭露（A→B→C）。

Ch4 對應期末考終焉：5 個路人全 isQuestGiver=false，依 chapter4.md 場景台詞定位。西裝學長雖在此表，但 `World::SpawnChapterNpcs` 的生成期過濾器會依 `Flag_ScoldedSenior`/`Flag_HelpedSenior` 決定是否實際生成他，把分流閘門置於生成器而非資料表。幕間（Interlude）與四種結局回傳空靜態向量。

## 關鍵內容（類別 / 函式 / 資料）

- `ChapterNpcSpawns(SemesterState state) → const std::vector<NpcSpawn>&`：inline 分派函式，以 switch-on-SemesterState 回傳對應靜態向量。
- `kChapter2`：Ch2 6 個 NPC，管理員為唯一 isQuestGiver=true。
- `kChapter3`：Ch3 8 個 NPC，5 個路人＋3 個物物交換鏈給予者（vendor_sausage_a、loudspeaker_b、senior_c）。
- `kChapter4`：Ch4 5 個 NPC，全 isQuestGiver=false。
- `kInterlude`、`kEndingA/B/C`：空靜態向量，對應市集插曲與各結局無名冊的語意。

## 相依與在架構中的位置

- **#include（往外）**：`NpcSpawns.h`（`NpcSpawn` 結構體與 `DefaultNpcSpawns()` Ch1 委派）、`SemesterState.h`（分派 switch 鍵）
- **被誰使用（往內）**：`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（`RespawnChapterRoster`）；多個章節測試（`test_chapter_spawns.cpp`、`test_ch1_flavor_crowd.cpp`、`test_chapter2_roster.cpp`、`test_chapter3_roster.cpp`、`test_chapter4_roster.cpp`、`test_chapter_transitions.cpp` 等）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層純資料——由 World（Spawn 管線）在章節轉移時讀取以重建 NPC 容器；本身為無狀態 inline 查詢，不持有任何可變資料。

## OO 概念與設計重點

資料表驅動設計（Table-Driven Configuration）的典型應用：每章的 NPC 陣容獨立於 World，以章節狀態為鍵查表，消除 World 對各章 NPC 的直接耦合。備注中詳細說明各 NPC 位置的設計依據（場景台詞對應地理座標、跑道半徑計算、最近 NPC 間距），反映地形遮罩與可達性驗證的生產品質要求。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ChapterSpawns.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterSpawns.h) · [← 全檔索引](../files-index.md)
