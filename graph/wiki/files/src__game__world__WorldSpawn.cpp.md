---
id: file:src/game/world/WorldSpawn.cpp
type: source
path: src/game/world/WorldSpawn.cpp
domain: game
bucket: world
loc: 344
classes: []
sources: ["src/game/world/WorldSpawn.cpp"]
---
# `WorldSpawn.cpp`

> **一句定位**：`World` 的章節生成邏輯——`SpawnChapterNpcs`、`SpawnChapterQuestItems`，以及四個自我閘控的延後 `MaybeSpawn*` 輔助函式——全部為 `World` 成員，拆分以控制單檔長度。

## 職責

此檔屬於 game / world 層，是 `World.cpp` 的生成部分，與 `World.cpp` 共同編譯一個類別（成員宣告不重複）。

**`SpawnChapterNpcs(SemesterState)`**：依 `ChapterNpcSpawns(state)` 生成章節 NPC（過濾 Ch4 中若玩家曾斥責 `suit_senior` 且未補救則跳過），並依 `ChapterVendors(state)` 生成攤販（各攤以 `VendorSpriteFor` 挑選彼此分明的 sprite，解決舊版十個 shop_auntie 分身問題），再生成 `ChapterPickups(state)` 的硬幣與（部分章節的）`SpawnChapterQuestItems` 的任務旗標拾取物。Ch4 額外生成 `TrueUmbrella`（體育館後台 `Vec2{1640,375}`）和 DLC 告示牌（`Vec2{1305,88}`）。Ch3 生成 5 名跑者（`EnableCircularRun`，各相隔 72°）和 10 名閒置者（`EnableWander`），Ch1 生成加退選人群（`EnableWander`）和風味 NPC（載入 (a) 段台詞池）。所有物件都記入 `chapterRoster_`，確保章節結束時清掃。

**`SpawnChapterQuestItems(SemesterState)`**：從 `ChapterQuestItems(state)` 建立 `QuestFlagPickup` 物件並記入名冊。

**四個 `MaybeSpawn*` 函式（自我閘控，每次造訪一次性）**：
- `MaybeSpawnChapter2Notes()` — `Flag_Bookworm` 成立後生成 3 份散落筆記。
- `MaybeSpawnChapter1VictimUmbrella()` — `Flag_SuitSeniorChoiceMade` 成立後生成苦主透明傘（硬閘控 苦主→學長→傘 主線）。
- `MaybeSpawnChapter3Umbrella()` — `Flag_KnowsUmbrellaLoc` 成立後生成 Ch3 TrueUmbrella（位於 `kChapter3UmbrellaPos`，體育館左側可見處，修復舊版被體育館遮蓋問題）。
- `MaybeSpawnInterludeLibrarianReturn()` — 插曲段 + returnTo==Ch3 + 手持借傘 + 未歸還，生成圖書館歸還點 NPC（座標 `Vec2{820,560}`，中正圖書館南側）。

## 關鍵內容（類別 / 函式 / 資料）

- `World::SpawnChapterNpcs(SemesterState)` — 生成 NPC / 攤販 / 硬幣 / 任務物品，Ch3 人群，Ch1 加退選人群；記入 `chapterRoster_`。
- `World::SpawnChapterQuestItems(SemesterState)` — 建立 `QuestFlagPickup` 記入名冊。
- `World::MaybeSpawnChapter2Notes()` / `MaybeSpawnChapter1VictimUmbrella()` / `MaybeSpawnChapter3Umbrella()` / `MaybeSpawnInterludeLibrarianReturn()` — 各帶一次性旗標守衛的延後生成函式。
- `skipScoldedSenior` — Ch4 的「斥責學長後不出場」漣漪旗標。
- `vendorIdx` — 確保 `VendorSpriteFor` 的索引不重複（十個不同人物）。
- `kCrowd[15]` — Ch3 操場人群 sprite 路徑表（male_01/04/05/07/08/09/11 + female_02/04/05/06/07/10/11/12）。
- `kIdle[10]` — Ch3 閒置者座標。

## 相依與在架構中的位置

- **#include（往外）**：`World.h`（類別宣告）、多個任務標頭（`ChapterPickups`/`ChapterQuestItems`/`ChapterSpawns`/`ChapterVendors`/`Flags`/`NpcSpawns`/`PipoyaRoster`/`Chapter2Quest`/`Chapter3Quest`）、實體標頭（`CashPickup`/`DlcSign`/`NPC`/`Player`/`QuestFlagPickup`）、`GameObjectFactory.h`、`Vendor.h`/`VendorSprite.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；和 `World.cpp` 一起組成 `World` 類別；`MaybeSpawn*` 由 `GameController` 在每幀偵測條件時呼叫。
- **繼承 / 實作 / 體現**：皆為 `World` 的成員。
- **每幀管線 / MVC 角色**：Model 層生成邏輯；`SpawnChapterNpcs` 在章節轉移時執行，`MaybeSpawn*` 在每幀 Spawn 步驟中按旗標觸發。

## OO 概念與設計重點

四個 `MaybeSpawn*` 函式統一採用「一次性旗標守衛（self-gating）」模式，確保延後生成邏輯等冪：無論 Controller 每幀呼叫多少次，物件只會生成一次。`skipScoldedSenior` 展示了「在生成時過濾」而非「在互動時隱藏」的設計取向，讓後續所有查詢（對話開啟 / Quest hook）對不存在的物件自然失效，無需額外條件分支。Ch3 人群的跑者以 `EnableCircularRun` 搭配不同初始角度（72° 間隔 + 不同速度）實現視覺多樣性，完全不新增美術資源。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/WorldSpawn.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/WorldSpawn.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Factory](../concepts/pat-factory.md) · [MVC](../concepts/arch-mvc.md)
