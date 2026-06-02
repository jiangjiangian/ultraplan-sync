---
id: file:tests/quest/test_chapter4_senior_skip.cpp
type: test
path: tests/quest/test_chapter4_senior_skip.cpp
domain: tests
bucket: quest
loc: 113
classes: []
sources: ["tests/quest/test_chapter4_senior_skip.cpp"]
---
# `test_chapter4_senior_skip.cpp`

> **一句定位**：驗證 `World::RespawnChapterRoster` 在 Ch4 的名冊過濾：`Flag_ScoldedSenior` 使西裝學長缺席，`Flag_HelpedSenior` 讓他回歸，且此過濾僅限 Ch4、不外溢到其他章節。

## 職責

此測試檔以 `World` 實例端到端驗證章節名冊過濾邏輯，測試的是 `RespawnChapterRoster` 在 Ch4 的特殊行為：依兩個旗標決定是否生成 `suit_senior`。

核心測試設計有四個 SUBCASE：
1. 僅 `kFlagScoldedSenior`：學長消失，其餘 4 個原型仍在。
2. `kFlagScoldedSenior` + `kFlagHelpedSenior`（Ch2 修補）：學長回歸，5 個原型全在。
3. 兩個旗標都沒有（預設）：5 個原型都在。
4. 僅 `kFlagHelpedSenior`（從未翻臉）：學長仍在。

第二個 TEST_CASE 驗證過濾的章節邊界：在同一個 `World` 上依序切換到 Ch2 → Ch3 → Ch4 → 回 Ch3，確認 Ch2/Ch3 不受影響（學長始終在），只有 Ch4 才讓他消失，且回到 Ch3 後他又出現（無名冊殘留）。

## 關鍵內容（類別 / 函式 / 資料）

- `HasNpcId(const World&, const char*)` ：匿名 namespace helper，在 `w.Objects()` 中找 npcId 匹配的物件。
- `TEST_CASE("Flag_ScoldedSenior 把 suit_senior 從 Ch4 隱藏")`：四個 SUBCASE 覆蓋所有旗標組合。
- `TEST_CASE("過濾僅限 Ch4 — 其他章節仍保留 suit_senior")`：往返切換序列，驗證名冊抽換後無殘留。
- `World("", /*loadSprites=*/false)`：不載入材質圖的測試模式建構。
- `World::RespawnChapterRoster(SemesterState)`：被測方法，在指定章節重新生成 NPC 名冊。

## 相依與在架構中的位置

- **#include（往外）**：`Flags.h`、`ChapterSpawns.h`、`GameObject.h`、`NPC.h`、`Player.h`、`SemesterState.h`、`World.h`。
- **被誰使用（往內）**：—（葉節點）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：— 測試 Model 層（`World`）的名冊抽換行為，對應每次章節轉場時 `SceneRouter::SettleRoster` 呼叫的效果。

## OO 概念與設計重點

「旗標組合驗證」是此類測試的主要技巧：四個互斥 SUBCASE 窮舉了兩個二進位旗標的四種組合，確保過濾邏輯的布林表達式不存在漏洞。往返測試（Ch4 消失 → 回 Ch3 又出現）是 mark-then-sweep 物件移除機制（`isActive_=false` + `Sweep()`）正確性的間接確認。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_chapter4_senior_skip.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_chapter4_senior_skip.cpp) · [← 全檔索引](../files-index.md)
