---
id: file:include/game/quest/ChapterQuestItems.h
type: header
path: include/game/quest/ChapterQuestItems.h
domain: game
bucket: quest
loc: 109
classes: [QuestItemPlacement]
sources: ["include/game/quest/ChapterQuestItems.h"]
---
# `ChapterQuestItems.h`

> **一句定位**：以 `SemesterState` 為鍵回傳各章節應生成的任務拾取物（任務旗標觸發型道具），是 World 的 `SpawnChapterQuestItems` 的純資料來源。

## 職責

`ChapterQuestItems.h` 定義 `QuestItemPlacement` 結構體與 `ChapterQuestItems(SemesterState)` inline 函式，集中管理各章節場景中散落的可拾取任務物品的空間配置與觸發語意。

Ch1 提供苦主的透明傘（`Flag_HasVictimUmbrella`），座標 (1700, 1610) 位於集英樓正南、西裝學長附近。此傘「撿起但不結束 Ch1」——實際章節結束是 `TryReturnVictimUmbrella` 歸還動作。Ch2 提供三頁散落筆記（`Flag_FoundNote1/2/3`），分別置於法學院西北 (450, 850)、集英樓東南 (1400, 1250)、校友服務中心南側 (1040, 1640)；三頁全撿後由 `QuestFlagPickup::OnPickup` 授予學霸 (b) +3 業力一次。

Ch2 筆記使用「依數量」台詞（`countMessages` / `kNoteMsgs`）：依玩家此刻手上已有幾頁決定「第一張／第二頁／最後一頁」文案，而非依哪一頁的身分——修正了舊設計中先撿 note3 卻誤印「最後一頁」的問題。Ch3–Ch4 及所有結局回傳空向量 `kNone`，由 World 對應的生成管線保證不生成任何任務道具。

全部座標均已通過碰撞遮罩的「嚴格可走」與「flood 可達」驗證（Ch1 傘點另特別測試過東側淨空走廊可供測試腳本行走）。

## 關鍵內容（類別 / 函式 / 資料）

- `struct QuestItemPlacement`：任務物品配置 POD，含世界座標 `pos`、拾取設置的旗標 `flag`、顯示訊息 `message`、完成集合 `completionFlags`、業力 `completionKarma`，以及選用的依數量台詞 `countMessages`。
- `ChapterQuestItems(SemesterState state) → const std::vector<QuestItemPlacement>&`：以章節狀態為鍵的 inline 分派函式；用 `static` 區域變數快取 `kChapter1`、`kChapter2`、`kNone`。
- `kChapter1`：Ch1 的透明傘，單一旗標 `kFlagHasVictimUmbrella`，completionKarma=0（業力在歸還時授予）。
- `kChapter2`：三頁筆記，三筆共用同一 `kNoteSet` 完成集合與 `kNoteMsgs` 依數量台詞，completionKarma=3。
- `kNoteSet`/`kNoteMsgs`：三頁筆記完成判定的旗標集合與對應台詞，共用於三筆配置。

## 相依與在架構中的位置

- **#include（往外）**：`Chapter1Quest.h`（`kFlagHasVictimUmbrella`、`kFlagFoundNote*` 常數）、`Chapter2Quest.h`（同上）、`SemesterState.h`（`SemesterState` 列舉與分派 switch）、`Vec2.h`（世界座標）
- **被誰使用（往內）**：`src/game/world/World.cpp`、`src/game/world/WorldSpawn.cpp`（在 `SpawnChapterQuestItems` / `MaybeSpawnChapter2Notes` 中讀取）；多個 quest 測試（`test_ch1_quest.cpp`、`test_chapter_questitems.cpp`、`test_chapter_spawns.cpp`、`test_spawn_reachability.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層純資料——章節名冊資料，由 World（Spawn 管線）讀取；本身不執行任何邏輯，僅定義配置表。

## OO 概念與設計重點

本檔是典型的「資料表驅動設計」：把任務物品配置從 `World` 的生成邏輯抽離，以靜態區域變數快取的 inline 函式提供「以章節為鍵的不可變配置向量」，消除重複字面值並讓測試直接斷言配置內容而不依賴 World 建構。

`countMessages` 欄位體現了對「拾取順序無關的全局計數」語意的精確建模：三頁筆記中任意一頁都能感知「現在是第幾頁被撿起」，讓玩家不論以何種順序探索都得到正確的敘事回饋——這是對錯誤設計的有意修正，並以代碼注釋明確說明。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/quest/ChapterQuestItems.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/ChapterQuestItems.h) · [← 全檔索引](../files-index.md)
