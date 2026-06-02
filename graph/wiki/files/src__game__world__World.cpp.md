---
id: file:src/game/world/World.cpp
type: source
path: src/game/world/World.cpp
domain: game
bucket: world
loc: 146
classes: []
sources: ["src/game/world/World.cpp"]
---
# `World.cpp`

> **一句定位**：`World` 的建構式、`RespawnChapterRoster`（章節名冊重生）與 `Sweep`（幀末延遲刪除）三個核心 Model 操作的實作，維持「front 即玩家、安全刪除、一次性旗標重新武裝」等不變式。

## 職責

此檔屬於 game / world 層，是整個遊戲 Model（`World`）最核心的三個功能。

**建構式 `World::World(...)`**：以 `GameObjectFactory` 建立玩家（`Vec2{500, 1860}`）及三把道德抉擇傘（`FragileUmbrella`/`ProfessorTrapUmbrella`/`CursedUmbrella`），再建立第一章的加退選申請書（`QuestFlagPickup`，`kFlagFoundForm`），快取玩家指標（`player_ = static_cast<Player*>(objects_.front().get())`），呼叫 `RespawnChapterRoster(semester_.Current())` 生成第一章 NPC 名冊，載入像素精確的地形碰撞遮罩（`LoadTerrainMask()`），最後在遮罩之後生成環境路人（漫遊 NPC），每個給予不同的 LCG 亂數種子（以 `0x1664525 * seed + 0x1013904223` 遞進）避免同步步伐。

**`RespawnChapterRoster`**：先從 `chapterRoster_` 建立裸指標集合，清空追蹤容器後以 `remove_if` 從 `objects_` 中移除對應的 `unique_ptr`（確保裸指標不在其 `unique_ptr` 釋放後被解參考）。接著重新武裝四個一次性旗標（`ch2NotesSpawned_`、`ch3UmbrellaSpawned_`、`ch1VictimUmbrellaSpawned_`、`interludeReturnSpawned_`），最後呼叫 `SpawnChapterNpcs(state)` 重建新章節的名冊。assert 確認元素 0 仍是玩家。

**`Sweep`**：以「先快照 `playerWillDie`、再 erase、最後依快照行動」的三步驟規避 `unique_ptr` 釋放後懸空指標問題（heap-use-after-free）。只移除 `IsActive() == false` 的物件，是 mark-then-sweep 延遲刪除模式的 sweep 端。

## 關鍵內容（類別 / 函式 / 資料）

- `World::World(string playerSpritePath, bool loadSprites, WorldOptions opts)` — 建構式；以 `GameObjectFactory` 建立場景物件，快取 `player_`，呼叫 `RespawnChapterRoster`，載入地形遮罩，生成環境路人。
- `World::RespawnChapterRoster(SemesterState)` — 安全移除舊章節名冊後呼叫 `SpawnChapterNpcs`；重新武裝四個一次性 spawn 旗標。
- `World::Sweep()` — 幀末延遲刪除；先快照 `playerWillDie`，再 erase，最後依快照清 `player_`。
- `player_` — 裸指標快取（`front()` 不變式，以 assert 確認）。
- `chapterRoster_` — 記錄本章節所有由 `Respawn` 管理的物件裸指標（NPC / 攤販 / 硬幣 / 任務物品）。
- `ch2NotesSpawned_` / `ch3UmbrellaSpawned_` / `ch1VictimUmbrellaSpawned_` / `interludeReturnSpawned_` — 各延後生成的一次性旗標，`RespawnChapterRoster` 重新武裝。

## 相依與在架構中的位置

- **#include（往外）**：`World.h`（類別宣告）、`GameObjectFactory.h`（建立初始物件）、各實體標頭（`CashPickup`/`DlcSign`/`NPC`/`QuestFlagPickup`）、任務標頭（`Chapter2Quest`/`Chapter3Quest`/`ChapterPickups`/`ChapterQuestItems`/`ChapterSpawns`/`ChapterVendors`/`NpcSpawns`/`PipoyaRoster`）、`Vendor.h` / `VendorSprite.h`、`Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）；`World` 由組裝根（`App`）持有，所有系統透過 `const World&` 或 `World&` 存取。
- **繼承 / 實作 / 體現**：`World` 是 MVC 的 Model，不繼承任何介面。
- **每幀管線 / MVC 角色**：Model 根；每幀管線末端呼叫 `Sweep()`；`RespawnChapterRoster` 在章節轉移時由 Controller 觸發。

## OO 概念與設計重點

`Sweep` 的「先快照再刪除」設計是 C++ 物件生命週期管理的典型防禦模式（[RAII](../concepts/oo-raii.md)）：`unique_ptr` 銷毀時指標即懸空，故必須在 erase 之前決定要做什麼。`RespawnChapterRoster` 的「建集合 → 清追蹤 → remove_if 只比對身分」序列確保迭代器安全，避免邊迭代邊刪除的 UB。`WorldOptions` 無障礙旗標的注入（`reducedMotion_` / `largeTargets_`）保持 `World` 建構式對環境的純粹性（環境讀取集中在 `ReadWorldOptionsFromEnv`，由組裝根呼叫一次），符合 [DIP](../concepts/arch-dip-renderer.md)。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/world/World.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/world/World.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[RAII](../concepts/oo-raii.md) · [MVC](../concepts/arch-mvc.md) · [Factory](../concepts/pat-factory.md)
