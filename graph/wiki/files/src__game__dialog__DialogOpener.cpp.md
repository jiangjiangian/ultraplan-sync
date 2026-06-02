---
id: file:src/game/dialog/DialogOpener.cpp
type: source
path: src/game/dialog/DialogOpener.cpp
domain: game
bucket: dialog
loc: 448
classes: [DispatchEntry]
sources: ["src/game/dialog/DialogOpener.cpp"]
---
# `DialogOpener.cpp`

> **一句定位**：NPC 對話的路由核心——依（章節、NPC id、玩家旗標/業力）決定開啟哪個子狀態台詞或選項選單，並套用第一章一次性獎勵。

## 職責

本檔在 `nccu` 命名空間內實作 `OpenNpcDialog`（兩個重載）、`OpenNpcDialogSub`、`ResolveOpenerSubState` 四個公開函式，是整個 NPC 互動系統的路由大腦。

**分派表架構**：20 個 `DispatchEntry`（`struct { SemesterState, string_view npcId, OpenerResolver }`）組成 `kDispatch` constexpr 陣列，每個條目對應一個函式指標（`int(*)(const Player&)`），例如 `Ch1Ta`、`Ch2Bookworm`、`Ch4SuitSenior` 等，根據玩家旗標決定要路由到哪個子狀態（0=基準開場，1=b, 2=c, 3=d）。`ResolveOpenerSubState` 對陣列做線性掃描，O(N)，N≤20。

**選項開場與只有台詞的開場**：`UsesChoiceOpener(npcId, state)` 判斷此 NPC 在此章節是否呈現分支選單（第一章的 `suit_senior`、`victim`、`shop_auntie` 三者）。有選單的 NPC，從所有 `subState>=1` 的 `SubEntry` 取出 `DialogChoice` 並交給 `dlg.Open(openerLines, choices)` 開啟；只有台詞的則直接 `dlg.Open(lines)` 開啟。

**硬閘控**：第一章西裝學長在 `Flag_PromisedVictim` 之前不呈現選項（直接播「打發走」台詞）；第二章學霸在 `Flag_MetLibrarian` 之前播「先去問管理員」台詞；西裝學長選項一旦確認（`Flag_SuitSeniorChoiceMade`）便退回只有台詞的回顧，防止漣漪旗標被堆疊。

**第四章助教終局（程式碼建構選項）**：若為 `Chapter4_Finals` + `npcId=="ta"`，整段邏輯由程式碼手工建立兩個選項（「體諒助教的辛勞」+15karma+`Flag_ConsoledTA`、「質問/強硬索回」-5karma）加上退出標籤。`Flag_TaFinaleChoiceMade` 作為一次性鎖，確認後降級為只有台詞的回顧；退出選項（`kDialogExitLabel`）不設此旗標，使玩家可暫離後再接觸。

**一次性副作用**：在 Chapter1 回顧子狀態下，若命中的 `SubEntry.setsFlag` 非空、`flagValue==true` 且玩家尚未持有，即套用 `karmaDelta` 與 `SetFlag`（助教申請書 / 苦主承諾的獎勵回顧）。此防護限定於第一章，避免其他章節的漣漪旗標被誤套。

## 關鍵內容（類別 / 函式 / 資料）

- `struct DispatchEntry { SemesterState; string_view npcId; OpenerResolver }`：分派表條目。
- `kDispatch`：20 個 `constexpr DispatchEntry`，覆蓋 Ch1–Ch4 的所有需路由 NPC。
- `ResolveOpenerSubState(npcId, state, Player&) -> int`：線性掃表，回傳目標子狀態索引。
- `OpenNpcDialogSub(DialogState&, npcId, state, subState)`：直接依指定子狀態開啟台詞。
- `OpenNpcDialog(DialogState&, npcId, state)`：無玩家的重載，只決定選單 vs 台詞。
- `OpenNpcDialog(DialogState&, Player&, npcId, state)`：完整重載，含硬閘控與一次性副作用。
- `UsesChoiceOpener(npcId, state)` → `bool`：判斷是否呈現分支選單。
- 各章解析器函式：`Ch1Ta` / `Ch1Victim` / `Ch2SuitSenior` / `Ch2Ta` / `Ch2Librarian` / `Ch2Bookworm` / `Ch2ShopAuntie` / `Ch2Victim` / `Ch3Bookworm` / `Ch3Ta` / `Ch3Victim` / `Ch3SuitSenior` / `Ch3VendorSausage` / `Ch3Loudspeaker` / `Ch3SeniorC` / `Ch4SuitSenior` / `Ch4Bookworm` / `Ch4Ta` / `Ch4Victim` / `Ch4ShopAuntie`。
- `kDialogExitLabel`：「我再想想…」退出選項的字串常數（引自 `DialogState.h`）。

## 相依與在架構中的位置

- **#include（往外）**：`DialogOpener.h`、`Chapter2Quest.h` / `Chapter3Quest.h` / `Chapter4Quest.h`（各章旗標常數）、`DialogState.h`（`dlg.Open` 等方法）、`DialogSource.h`（`nccu::dialog::Entries`）、`Player.h`（旗標 / 業力查詢）。
- **被誰使用（往內）**：—（葉節點；由 `GameController` 的互動鉤子呼叫）。
- **繼承 / 實作 / 體現**：—（純自由函式）。
- **每幀管線 / MVC 角色**：Controller 層的互動觸發點；在玩家按 E 觸發 `RunInteractHooks` 時呼叫，屬每幀管線中「E 互動 `RunInteractHooks`」階段。

## OO 概念與設計重點

本檔以「查表取代巨型 switch」重構：原 200 行 switch 改寫為 20 列分派表，是 [Strategy 模式](../concepts/pat-strategy.md) 的一種資料驅動變體——每個 `OpenerResolver` 函式指標封裝了一個 NPC-章節組合的路由策略，呼叫端（`ResolveOpenerSubState`）無需知道具體策略。程式碼建構選項（助教終局）示範了 Template Method 與資料驅動之間的邊界：風味台詞來自內容檔，但選項結構（業力/旗標/退出標籤）由程式碼負責。

## 連結

[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/game/dialog/DialogOpener.cpp) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/game/dialog/DialogOpener.cpp) · [← 全檔索引](../files-index.md) · 相關概念：[Strategy](../concepts/pat-strategy.md) · [State](../concepts/pat-state.md)
