---
id: file:tests/quest/test_ch1_flavor_crowd.cpp
type: test
path: tests/quest/test_ch1_flavor_crowd.cpp
domain: tests
bucket: quest
loc: 153
classes: []
sources: ["tests/quest/test_ch1_flavor_crowd.cpp"]
---
# `test_ch1_flavor_crowd.cpp`

> **一句定位**：驗證 Ch1 搶課人潮與靜態風味 NPC 為附加內容——不擋主線、不設旗標、台詞確定性循環、Player 始終在 `objects_` 最前端。

## 職責

此測試驗證 Ch1 的「風味層」（flavor layer）不會破壞主線約束。具體驗證三件事：

1. **搶課人潮的生成與 Player 不變量**：`Chapter1CrowdSpawns()` 中的所有座標都有對應的無 npcId NPC 出現在 `World.Objects()` 中；無論人潮有無，`objects_.front()` 始終是 Player（不變量）；進入市集過場人潮消失，回到 Ch1 後人潮回來，不變量兩者皆守住。

2. **靜態風味 NPC 的特性**：宣告在 `Chapter1FlavorSpawns()` 中的每個 NPC 都「在名冊中、不是任務給予者（`IsQuestGiver()==false`）、不在主線錨點集合中」，且 `IsChapter1FlavorNpc(id)` 為 true；主線 id（victim、suit_senior 等）不是風味 NPC。

3. **確定性台詞循環**：風味 NPC 的對話池（`ch1_flavor_grab` 在 chapter1.md 的 (a) 段落）每次 `Interact()` 推進一行，依池大小取模循環，兩輪序列相同；兩個以相同種子初始化的 NPC 序列也相同（無 RNG）。互動永不改動玩家的業力或旗標。

## 關鍵內容（類別 / 函式 / 資料）

- `CountCh1Crowd(World&)`：計算坐落在 `Chapter1CrowdSpawns()` 生成點上且 npcId 為空的 NPC 數量。
- `FindNpc(World&, id)`：在 World 中線性搜尋指定 npcId 的 NPC。
- `TEST_CASE("Ch1 加退選搶課人潮會生成，且 Player 維持在最前端")`：驗證人潮數量 ≥ `Chapter1CrowdSpawns().size()`；前端不變量；離開/返回 Ch1 的生命週期。
- `TEST_CASE("Ch1 風味 NPC 為靜態、非任務、不在主線上")`：對每個 `Chapter1FlavorSpawns()` 條目驗證三項特性，並反向驗證主線 id 和空 id。
- `TEST_CASE("風味 NPC 以確定性方式循環台詞池，且不設任何旗標")`：載入 `ch1_flavor_grab` 池，跑兩輪並比較序列；兩個 NPC 起點相同序列相同；玩家業力與旗標均不變。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/NpcSpawns.h`（`Chapter1CrowdSpawns`、`Chapter1FlavorSpawns`、`IsChapter1FlavorNpc`），`game/quest/ChapterSpawns.h`，`game/dialog/DialogSource.h`，`game/entities/NPC.h`，`game/entities/Player.h`，`engine/core/GameObject.h`，`game/world/World.h`，`game/state/SemesterState.h`，`engine/math/Vec2.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純資料邏輯測試）

## OO 概念與設計重點

`objects_.front() == Player` 的不變量測試是 World Model 設計的架構規格：Player 必須始終在物件清單的最前端，這影響 `ForEachRole` 的走訪順序和某些優先處理邏輯。確定性台詞循環（無 RNG）使 harness 腳本的對話 advance 可預測，是 [harness 架構](../concepts/arch-harness.md) 可靠性的基礎。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch1_flavor_crowd.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch1_flavor_crowd.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md)
