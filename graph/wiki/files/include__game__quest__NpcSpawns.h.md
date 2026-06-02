---
id: file:include/game/quest/NpcSpawns.h
type: header
path: include/game/quest/NpcSpawns.h
domain: game
bucket: quest
loc: 161
classes: [NpcSpawn]
sources: ["include/game/quest/NpcSpawns.h"]
---
# `NpcSpawns.h`

> **一句定位**：定義 `NpcSpawn` 結構體與 Ch1 預設 NPC 名冊、環境路人名冊、Ch1 群眾名冊和 Ch1 風味 NPC 名冊，以及 `IsChapter1FlavorNpc` 判斷函式。

## 職責

`NpcSpawns.h` 是 game 層 NPC 生成配置的基礎定義檔，提供 `NpcSpawn` POD 結構體與四個以 `inline` 函式為包裝的靜態名冊，供 `ChapterSpawns.h` 委派（Ch1）與 World 直接使用（環境路人）。

`NpcSpawn` 結構體包含世界座標 `pos`、sprite 路徑 `spritePath`、對話鍵 `npcId`、任務給予者旗標 `isQuestGiver`（顯示「!」），以及遊走旗標 `wander`（預設 false）。每個 NPC 的位置設計為停在其錨點建築碰撞矩形「之外」，使玩家靠近對話不被牆推開。

`DefaultNpcSpawns()` 提供 Ch1 的 5 個設計原型：苦主（綜合院館西南 1660,1010，isQuestGiver=true）、西裝學長（集英樓正南 1620,1560）、學霸（中正圖書館前 820,545）、助教（行政大樓前 1220,775）、福利社阿姨（樂活小舖 1560,1560）。每個座標均有詳細遮罩驗證記錄。

`AmbientStudentSpawns()` 提供 6 個環境路人（wander=true，無 npcId），沿指南路散布讓校園顯得有人氣。`Chapter1CrowdSpawns()` 提供 8 個 Ch1 加退選群眾（wander=true，無對話），散布於中央校園 y≈860–1180，與任務錨點保持距離。`Chapter1FlavorSpawns()` 提供 3 個固定式風味 NPC（wander=false，有 npcId：`ch1_flavor_grab`、`ch1_flavor_rain`、`ch1_flavor_bag`），使玩家能聽到各色搶課抱怨。

`IsChapter1FlavorNpc(string_view)` 由 `Chapter1FlavorSpawns()` 推導，讓 GameController 的 E 互動分派把風味 NPC 導向循環 `NPC::Interact()` 路徑，「結構性地」保證風味 NPC 不會執行主線鉤子或設任務旗標。

## 關鍵內容（類別 / 函式 / 資料）

- `struct NpcSpawn`：生成 POD，含 `pos`（Vec2）、`spritePath`（const char*）、`npcId`（const char*）、`isQuestGiver`（bool）、`wander`（bool，預設 false）。
- `DefaultNpcSpawns() → const vector<NpcSpawn>&`：Ch1 5 個主線/路人 NPC，靜態快取。
- `AmbientStudentSpawns() → const vector<NpcSpawn>&`：6 個環境路人，全 wander=true。
- `Chapter1CrowdSpawns() → const vector<NpcSpawn>&`：8 個 Ch1 搶課群眾，全 wander=true，無對話。
- `Chapter1FlavorSpawns() → const vector<NpcSpawn>&`：3 個固定式風味 NPC（ch1_flavor_grab/rain/bag），wander=false。
- `IsChapter1FlavorNpc(string_view npcId) → bool`：由 `Chapter1FlavorSpawns()` 推導的集合成員判定，確保風味 NPC 與主線路徑結構性隔離。

## 相依與在架構中的位置

- **#include（往外）**：`Vec2.h`（世界座標）
- **被誰使用（往內）**：`include/game/quest/ChapterSpawns.h`（委派 Ch1 名冊）、`src/game/controller/GameController.cpp`（`IsChapter1FlavorNpc` 互動分派）、`src/game/controller/InteractDispatch.cpp`、`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`；測試（`test_ch1_flavor_crowd.cpp`、`test_ch1_spine_reachable.cpp`、`test_spawn_reachability.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層純資料——建構期（World 初始化）時由 SpawnChapterNpcs 讀取以生成 NPC 物件；`IsChapter1FlavorNpc` 在 E 互動時由 Controller 呼叫。

## OO 概念與設計重點

`IsChapter1FlavorNpc` 是「結構性保證」設計的好例子：透過分派層面的 id 檢查，使風味 NPC 與主線鉤子完全隔離，而不依賴每個鉤子函式內的防禦性判斷。靜態區域 `kAll` 向量確保名冊只建構一次（惰性初始化），而各名冊詳細的座標遮罩驗證記錄（自身與四鄰可走性、flood 可達性）體現了高品質的測試/驗證文化。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/NpcSpawns.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/NpcSpawns.h) · [← 全檔索引](../files-index.md)
