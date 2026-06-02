---
id: file:tests/quest/test_ch2_reactive_substates.cpp
type: test
path: tests/quest/test_ch2_reactive_substates.cpp
domain: tests
bucket: quest
loc: 135
classes: []
sources: ["tests/quest/test_ch2_reactive_substates.cpp"]
---
# `test_ch2_reactive_substates.cpp`

> **一句定位**：驗證 Ch2 四段反應式台詞已改寫為真正的旗標分支子狀態，且各台詞只在其對應旗標下可達——其他情況不得洩漏。

## 職責

此測試驗證 `chapter2.md` 的改寫成果：原本以行內標註撰寫、DialogLoader 會默默丟棄的「*（若 Flag_X = true）*」反應式台詞，已改寫為受旗標守門的獨立子狀態，由 `ResolveOpenerSubState` 路由。

三個 TEST_CASE 從三個層次驗證：

1. **改寫後的 MD 確實載入**：`SubHasLine()` 搜尋四組（學霸 (b)、福利社阿姨 (b)、苦主 (c)/(d)）的子狀態與台詞對應。
2. **路由正確性**：用 `ResolveOpenerSubState` 驗證三個 NPC 在各種旗標組合下的子狀態選擇，並驗證競爭旗標（ProfessorTrap 優先於 HelpedTA）與優先序（承諾勝出醜傘）。
3. **端到端開啟**：`OpenNpcDialog` 端到端驗證（a）詛咒傘路徑的學霸 (b) 能抵達詛咒台詞；（b）苦主承諾 (c) 能抵達承諾回呼；（c）無旗標時預設 (a) 不含詛咒台詞（洩漏防護）。

## 關鍵內容（類別 / 函式 / 資料）

- `SubHasLine(npc, sub, needle)`：搜尋指定 NPC 的 sub 號子狀態是否含 needle 台詞行。
- `TEST_CASE("chapter2.md 改寫後的反應式子狀態確實載入對應台詞")`：四組台詞的存在性驗證。
- `TEST_CASE("Ch2 路由只在各自旗標下才抵達對應的反應式子狀態")`：三個 SUBCASE（學霸、福利社阿姨、苦主）的完整路由表驗證。
- `TEST_CASE("OpenNpcDialog 端到端開啟對應的反應式台詞")`：三個 SUBCASE（詛咒 (b)、承諾 (c)、無旗標預設 (a)），後兩個特別包含硬性關卡前提（`kFlagMetLibrarian`、`kFlagPromisedVictim`）。

## 相依與在架構中的位置

- **#include（往外）**：`game/quest/Flags.h`、`Chapter2Quest.h`，`game/dialog/DialogOpener.h`、`DialogSource.h`、`DialogState.h`，`game/entities/Player.h`。
- **被誰使用（往內）**：—（葉節點 / 組裝根）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：—（純對話邏輯測試）

## OO 概念與設計重點

「反應式子狀態」的設計讓對話內容依旗標動態切換，而不在 DialogLoader 層做字串替換——這是更清晰的關注點分離。「洩漏防護」測試（無旗標時詛咒台詞不出現）確保子狀態之間的互斥性。`SUBCASE` 的使用讓相關測試共用 `SetContentDir/Reload` 的 setup 但各自獨立斷言。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:tests/quest/test_ch2_reactive_substates.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/tests/quest/test_ch2_reactive_substates.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[State](../concepts/pat-state.md)
