---
id: file:tests/quest/test_ch1_spine_reachable.cpp
type: test
path: tests/quest/test_ch1_spine_reachable.cpp
domain: tests
bucket: quest
loc: 227
classes: [SpineResult]
sources: ["tests/quest/test_ch1_spine_reachable.cpp"]
---
# `test_ch1_spine_reachable.cpp`

> **一句定位**：在實際出貨的碰撞遮罩上端到端驗證 Ch1 最小互惠主線可抵達 Chapter 2，並確認路線在兩次執行間具重播確定性。

## 職責

此測試是整個 codebase 中最複雜的整合測試之一，直接驅動真正的 `ScriptInput + GameController`（與 Harness 執行順序相同），驗證以下端到端不變式：

1. **南牆缺口幾何**：實際的 `collision_mask.png` 在 y≈1761–1819 有一道東西向牆，其唯一縱向缺口在 x≈880–1042。舊路線（x=320/560/750 等縱列）都會貼牆卡住。
2. **最小 Ch1 主線路線**：從出生點 (500,1860) 出發，穿過 x≈1041 缺口，依序：(a) 在綜合院館見苦主承諾、(b) 在集英樓找西裝學長做選擇（讓雨傘出現）、(c) 撿起苦主之傘、(d) 帶傘回去授予並讀完交換對話、(e) 從市集離開到 Ch2。最終 karma==60（苦主+5、學長善意+5）。
3. **確定性**：兩次用相同腳本 `kSpineScript` 執行，`SpineResult` 的所有欄位（semester、flags、karma、frames）完全相等。

## 關鍵內容（類別 / 函式 / 資料）

- `SpineResult`：儲存一次主線執行後的可觀察狀態：`semester`、`promisedVictim`、`hasTrueUmbrella`、`karma`、`frames`。
- `RunSpine(script, maxFrames)`：建立 World + GameController，跑腳本，回傳 `SpineResult`；內含 teardown。
- `kSpineScript`：244 行的完整最小主線腳本（含繞行路線的多個 goto、interact/choose/advance/wait），帶有詳盡的逐段注釋說明每個路段的設計理由。
- `TEST_CASE("出貨遮罩的南牆恰好只有 x≈880-1042 這個缺口")`：使用 `CollisionMask` 直接驗證舊有牆縱列（x=320/560/750/1140/1500/1560/1706）確實被擋、缺口縱列（x=1000）淨空、牆北走廊（y=1750）既無牆也無 NPC。若資產不存在則優雅降級（跳過幾何斷言）。
- `TEST_CASE("最小 Ch1 主線在出貨遮罩上可抵達 Chapter 2")`：斷言 `promisedVictim==true`、`hasTrueUmbrella==false`（進入 Ch2 時清除）、`karma==60`、`semester==Chapter2_Midterms`、`frames<9000`（未卡住）。
- `TEST_CASE("Ch1 主線路線在兩次執行間具確定性")`：`a.semester==b.semester` 等全部欄位，並再次斷言抵達 Ch2（排除「確定性卡住」）。

## 相依與在架構中的位置

- **#include（往外）**：`engine/platform/ScriptInput.h`、`game/controller/GameController.h`、`engine/events/EventBus.h`、`game/world/World.h`、`game/entities/Player.h`、`game/dialog/DialogState.h`、`game/dialog/DialogSource.h`、`game/quest/ChapterVendors.h`、`game/state/SemesterState.h`、`game/world/CollisionMask.h`、`game/quest/NpcSpawns.h`、`engine/math/Rect.h`、`engine/input/Input.h`、`engine/platform/Time.h`、`game/quest/Flags.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：直接驗整個遊戲管線（每幀 Advance→ResolvePlan→Update），包含 Movement、Collision、Spawn（`MaybeSpawnChapter1VictimUmbrella`）、Interlude 轉場。

## OO 概念與設計重點

此測試是「地圖不封鎖主線」的活文件：南牆缺口幾何的前提驗證（健全性測試）確保路線前提不會悄悄腐壞；主線路線本身如果被地圖變更封死、或硬性關卡走進死路，此測試便失敗。`kSpineScript` 的詳盡注釋說明了為何每個 goto 路段這樣走，使地圖修改者能理解約束來源。確定性重播與 [harness 架構](../concepts/arch-harness.md) 的 (計畫步驟, World快照) 純函式設計直接掛鉤。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch1_spine_reachable.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch1_spine_reachable.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Harness](../concepts/arch-harness.md) · [State](../concepts/pat-state.md)
