# Cycle 9 起手診斷 — 遊玩 UX bug 報告

**作者**: bughunter / cycle9 起手
**根據**: read-only 對 `src/`, `include/`, `docs/content/`, 三個結局 playtest
(`.claude/out/cycle9_diag_{a,b,c}/{state.jsonl,shots/}`)
**結論一句話**: NPC spawn 本身 OK,但 (a) interlude/ending 出現「stale 殘留」, (b) 章節
轉換/karma 變化/任務完成幾乎沒有 UI feedback, (c) 沒有 quest-giver 視覺指示符, (d)
四維道市集出口的「公告板 NPC」**被靜默 trigger zone 取代且沒有任何視覺暗示**。

---

## 1. NPC Spawn 缺陷(每章節)

對比方式: `docs/content/*.md` 的 `## NPC：` 標題 vs `state.jsonl
.objects.npcs` 抽取每章節進入後第一個非空幀。

| 章節 | 期望 NPC | 實際 spawn (Ending A) | 缺口/錯位 | 證據 |
|------|----------|----------|---------|------|
| Ch1 加退選 | victim, suit_senior, bookworm, ta, shop_auntie | 同左 | OK | `state.jsonl:frame=0` |
| Interlude 福利社 | (內容檔只有 `## 攤位：`,**沒有 `## NPC：`**;ChapterSpawns.h:22 `kInterlude` 為空) | **5 個 Ch1 archetype 全部殘留** (`victim/suit_senior/bookworm/ta/shop_auntie`) | **REAL drift**: 殘留是 RespawnChapterRoster 對空 roster 無動作 + 上一章 NPC `chapterRoster_` 沒被 mark inactive,因 `objects_` erase predicate 只移除 tracked 指針,留下舊生命週期 | A:frame=1765-2109, B:1324, C:915 |
| Ch2 期中考 | victim, suit_senior, bookworm, ta, shop_auntie, librarian | 同左 (lag 1 frame) | **1-frame 空幀**: `frame=2110 npcs=[]` → `frame=2111 npcs=[6]`(roster respawn 排在下一 tick,GameController.cpp:128 `lastRosterState_` polling 機制) | A:2110-2111 |
| Ch3 運動會 | + vendor_sausage_a, loudspeaker_b, senior_c | 同左 (lag 1 frame) | 同上 1-frame 空幀 | A:4014-4015 |
| Ch4 期末 | victim, suit_senior, bookworm, ta, shop_auntie | 同左 (lag 1 frame) | (1) 1-frame 空幀; (2) chapter4.md 補2 註記「Flag_ScoldedSenior 學長不出場」**未實作**,suit_senior 永遠 spawn (ChapterSpawns.h:78-80 「KNOWN OMISSION」) | A:5430 |
| Ending A | 苦主 only (chapter4.md:356 audit "Ending A roster `kEndingA` empty (`ChapterSpawns.h:96`, TODO)") | **5 個 archetype 全部殘留** | 同 Interlude:`kEndingA/B/C` 全空,但 roster 殘留;結局畫面理應由 `EndingView` 完全覆蓋世界,但若 EndingView 沒蓋住會看到 5 個 NPC 木雕一樣站著 | A:7462, B:4806, C:4114 |
| Ending B/C | 空 (應該都是空) | 同上 5 個殘留 | 同上 | — |

**根因**: `World::RespawnChapterRoster(state)` (`src/World.cpp:166-196`) 只移除 *上一次* 註冊到 `chapterRoster_` 裡的東西。Ch1 跳到 Interlude/Ending 時,如果 `ChapterNpcSpawns(state)` 回傳空 vector,則 `SpawnChapterNpcs` 不做事,而 Ch1 NPC 是經 `chapterRoster_` 註冊的(會被移除)。**所以** Interlude 應該是空——但**實際 state.jsonl 顯示 NPC 還在**!這代表 Ch1 第一批 NPC **沒進 chapterRoster_**(看 `World.cpp:63` constructor 直接創 NPC 並 push 到 `objects_` 但**沒** `chapterRoster_.push_back`)。確認:`World.cpp:74-76` `SpawnChapterNpcs` 才會 push 到 chapterRoster。ctor 走 `DefaultNpcSpawns()` 路徑 NOT 經 SpawnChapterNpcs → **永遠不會被清掉**。這是 BUG。

---

## 2. Discoverability 問題(玩家找不到)

證據幀請見對應 `.png`:

- **Ch3 三個交易攤位 NPC 位於 y=1850**,但玩家在 Ch3 開場(frame=4015 起)位置 ~y=1430(從中央傘箱位置回來),螢幕視窗 ~360px 高(畫面顯示 ~210px in scaled view),三個攤位是**離開視野下方** → 玩家必須亂走才能撞見。截圖證據:`.claude/out/cycle9_diag_a/shots/frame_004200.png`(玩家在 y=1713)無攤位可見;`frame_004800.png` 也沒看到交易攤主。
- **沒有任何 NPC 指示符**: `NPC::IsQuestGiver()` (`include/NPC.h:52`) 旗標**已存在且 spawn 時正確設定** (e.g. `NpcSpawns.h:32` victim isQuestGiver=true;`ChapterSpawns.h:64-67` Ch3 trade chain true) — 但 grep `View.cpp` 沒有任何 `IsQuestGiver()` 呼叫,**渲染層完全忽略這個旗標**。沒有 `!` 標示、沒有 arrow,沒有 minimap。
- **目標面板存在但太小**: `View.cpp:245-264` 在螢幕上方中央畫 `CurrentObjective()` 文字,字級 14px,深色背板。在 360x202px viewport(`/home/user/ultraplan-sync/.claude/out/cycle9_diag_a/shots/frame_001800.png`) 上,目標文只佔正中央一小條 — 截圖看起來像細條,易被忽略。已實作但 readability 弱。
- **Interlude 出口沒有任何標示**: `InterludeExit.h:13-19` 文件明說「公告板 NPC 已被 InterludeExit 取代」,但 `interlude_market.md:6,49` 仍敘述「公告板 NPC ... 對話選項...選擇『離開』」。實作:`GameController.cpp:511` `InInterludeExitZone()` 偵測 player.y>=1900 自動 set `Flag_LeaveInterlude`。**沒有 NPC、沒有 sprite 物件、沒有 trigger 範圍視覺化、沒有 ShowMessage 在玩家踩到時**(GameController.cpp:501-511 完全無 feedback)。 → 玩家只能靠 `QuestObjective.h:29` 「逛完往南離開」這句文字提示,而那是顯示在小目標條中央,**容易錯過**。
- **章節進入點位置不直觀**:`kInterludeEntry={500,1500}`(InterludeExit.h:22) 是中央羅馬廣場位置;玩家從 Ch1 結束位置(集英樓 320,1280)拿到傘 → 強制傳送到 1500.0 → 沒有「你已到達市集」訊息 + 沒有出口指引 + 上次 toast「你撿到了 TrueUmbrella」還掛在 HUD 上(state.jsonl frame=1765 `hud='你撿到了 TrueUmbrella'`,過 4 秒才淡掉)。

---

## 3. Quest Progression Feedback 缺口

- **沒有 QuestLog / ObjectiveTracker / QuestStateMachine class**:
  `grep ObjectiveTracker|QuestLog` → 0 hits in `src/include/`。 只有 `QuestObjective.h` 一個小 helper 純函數 `CurrentObjective(state, player)` 回傳一行字。 → 玩家無法看到 "已完成 / 進行中 / 未開始" 的 quest list。
- **章節 N → N+1 切換時無 ShowMessage**:`src/ChapterGate.cpp` 13-41 行只跑 `semester.Transition()` + `dialog.Close()`,**完全沒有 publish `EventType::ShowMessage`**。對照 state.jsonl(Ending A):
  - `frame=1765` Ch1→Interlude  `events=[UmbrellaClaimed/ShowMessage("你撿到了 TrueUmbrella")]` ← 是傘的 toast,不是章節切換
  - `frame=2110` Interlude→Ch2 `events=[]` ← **零事件**;HUD 仍掛著 25 frame 前的 TrueUmbrella toast
  - `frame=3690` Ch2→Interlude `events=[]` ← 零事件
  - `frame=4014` Interlude→Ch3 `events=[]` ← 零事件
  - `frame=5084` Ch3→Interlude `events=[UmbrellaClaimed/ShowMessage]` ← 仍是 TrueUmbrella toast 巧合
  - `frame=5430` Interlude→Ch4 `events=[]` ← 零事件
  - `frame=7462` Ch4→Ending  `events=[]` ← 零事件,**連結局都沒有 toast**(靠 EndingView 蓋畫面)
- **karma 變化沒有專屬 toast**:`CursedUmbrella.cpp:17` 是唯一一個 publish `KarmaChanged` 事件的點;但 grep `Subscribe.*KarmaChanged` 結果 = **0 subscribers**。`EventBus::Publish(KarmaChanged)` 永遠是死信。
- **HUD message 沒有清空機制**(World.h:106-108):`SetHudMessage()` 只覆蓋,**從不 reset 到空字串**。視覺上有 `kHudTtl=4s` 淡出,但 `state.jsonl.hud` 一直顯示最後一條 — 即使在 Ch4 結局前一秒,HUD 字段仍是 Ch3 拿到傘那句"你撿到了 TrueUmbrella"。玩家 UX 上會看到上一個事件的殘影。

---

## 4. Task-Completion Notification 缺口

- **沒有 Toast / Notification / Popup class**: 只有 `MessageView::DrawHudMessage()`(`include/MessageView.h`) 渲染最後一條 ShowMessage 字串,做了 4-second 淡出 — 是「最新事件」,不是「任務達成」獨立通道。
- **既有 events 中 `KarmaChanged` 沒有訂閱者**:
  - publish 站點:`CursedUmbrella.cpp:17` (only one!)
  - subscribe 站點:**0** (grep `Subscribe.*KarmaChanged` 全空)
  → 該事件設計了但沒被使用。其他 karma 變化(±N) 都是直接 `player.AddKarma()` 沒 publish。
- **沒有 `EventType::QuestCompleted` / `ChapterCleared` / `FlagSet`**:`EventBus.h:10-19` 只有 5 種事件(UmbrellaClaimed/KarmaChanged/ShowMessage/EnteredBuilding/PickupAcquired)。沒有抽象的「任務狀態變化」事件;每個 quest 完成都得自己塞一個 ShowMessage(常常忘記),或者根本不塞(如 ChapterGate.cpp 全章節轉換)。
- **QuestFlagPickup 的 ShowMessage 是內容資料**(`include/ChapterQuestItems.h:24`),每個 pickup 有自己的 `message`,但 ChapterGate / EndingGate / EventWiring 等系統級轉換**全部沒對應 message**。
- **Picture-side**:截圖 `frame_001800.png`(Interlude 進入 35 frame 後)畫面上**完全看不出**「你進入了市集」,也沒有任何閃光/震動。

---

## 5. Cycle 9 候選工作項(按優先序)

### H1 [Spawn-leak] Ch1 NPC 不在 chapterRoster_ → Interlude/Ending 殘留 5 個 NPC
- **入手點**: `src/World.cpp:63` ctor 路徑直接 push NPC 到 `objects_` 但**沒有** `chapterRoster_.push_back(npc.get())`。對比 `World.cpp:74-76` 的 `SpawnChapterNpcs` 路徑就有。
- **修法**: 統一走 `SpawnChapterNpcs(Chapter1_AddDrop)` 或 ctor 也 push 到 chapterRoster_。
- **回歸測試**: 新 doctest 直接 `world.RespawnChapterRoster(Interlude_Market)` 後 assert `world.Objects()` 之中找不到 isQuestGiver NPC。
- **預期工作量**: 5 行 ctor 改動 + 1 個 ~25 行 test。

### H2 [Feedback gap] 章節清關沒有 ShowMessage
- **入手點**: `src/ChapterGate.cpp:18,27,39` 三個 `semester.Transition(...)` 前後;`include/EventWiring.h:54,66` 兩個 EventBus subscriber 也跑 Transition。
- **修法**: 每個 transition 配對 `EventBus::Publish(Event{ShowMessage, "✓ 第N章 已完成 — 進入幕間市集"})`/「✓ 進入第N章」/「✓ 抵達結局」。或者新增 `EventType::ChapterChanged`(可選)。
- **回歸測試**: 新 doctest 在 Transition 後 assert World.HudMessage() 含「第」「章」字。
- **預期工作量**: ~15 行 + 5 個 transition site,加 ~30 行 test。

### H3 [Discoverability] Interlude 出口完全沒有視覺/文字暗示
- **入手點**: `InterludeExit.h:23-31` (kInterludeExitZone) + `GameController.cpp:501-511` (偵測即 set flag,無 feedback)。
- **修法**: (a) 進入 Interlude 時即 `ShowMessage("市集中央。逛完後從南端離開") `; (b) 玩家走進 EXIT zone 但**還沒**設 Flag 時(第一次 enter)發一條 `"準備離開市集..."` toast。 (c) 補:`View.cpp` 在 Interlude state 畫一個 ground marker(例如南端一條黃線/路標 sprite)。
- **回歸測試**: doctest 模擬玩家走到 y=1910 → 第一幀 assert World.HudMessage() 非空。
- **預期工作量**: ~20 行;ground marker 是純 View 改動 ~10 行;test ~30 行。

### H4 [Discoverability] NPC 沒有 quest-giver 指示符(`!` / arrow)
- **入手點**: `include/NPC.h:52` `IsQuestGiver()` 已存在但 View 不讀;NPC 已 SetSprite 但沒 overlay。
- **修法**: `View.cpp` draw NPC 後,如果 `n->IsQuestGiver() == true` 在 sprite 上方畫一個 16x16 的 `!` icon(rect + 文字)。或閃爍。
- **回歸測試**: 純 View 邏輯,測試在 spy renderer。
- **預期工作量**: ~20 行 View + ~30 行 test。

### H5 [Feedback gap] karma 變化沒對應 toast
- **入手點**: `src/Player.cpp` (AddKarma 是直接修改變數,大多數路徑不 publish KarmaChanged)。
- **修法**: 在 `AddKarma(delta)` 內部統一 publish `KarmaChanged` 事件 + 加一個 subscriber 在 World/View 顯示「業力 ±N」toast(可從 `kHudTtl=4s` 開短一點如 2s 避開覆蓋主敘事 toast,或新增 `EventType::KarmaToast` 走不同通道)。
- **預期工作量**: ~25 行 Player + 10 行 wiring + ~30 行 test。

### M6 [Stale doc] interlude_market.md 仍寫「公告板 NPC」,實作已換成 trigger zone
- **入手點**: `docs/content/interlude_market.md:6,49-57` 「公告板 NPC」描述;
   `docs/cycle8-audit/content/interlude_market.md` 也已標 #4 公告板 NPC 視覺 outstanding
- **修法**: 改 doc 對應「南端 trigger zone(已實作)」 + 標記為 INTENTIONAL Cycle-N redesign in CHANGELOG。
- **預期工作量**: doc only。

### M7 [Polish] Interlude/Ending roster 殘留(H1 副作用):chapter4.md「Flag_ScoldedSenior → 學長不出場」未實作
- **入手點**: `ChapterSpawns.h:81-93` Ch4 roster + 註記「KNOWN OMISSION」。
- **修法**: H1 修好後,Ch4 spawn 路徑加 if `player.HasFlag(Flag_ScoldedSenior) && !player.HasFlag(Flag_HelpedSenior)` skip suit_senior。
- **預期工作量**: ~10 行 + test。

### L8 [Polish] 1-frame respawn 空窗(玩家瞬間看到無 NPC 的新地圖)
- **入手點**: `GameController.cpp:127-129` (lastRosterState_ polling)。
- **修法**: 在 `semester.Transition()` 後同 frame 內(EndFrame 之前)就跑 RespawnChapterRoster — 而不是等下一 frame 開頭 poll。
- **預期工作量**: 5 行;但可能要小心 transition 中途 dialog/EventBus 還在 dispatch — 需 architecture-reviewer 確認 race。

### L9 [Polish] HudMessage 從不 reset → state.jsonl 永遠有 stale 字串
- **入手點**: `include/World.h:106-111`。
- **修法**: `Tick(dt)` 在 `hudAge_ >= kHudTtl` 時 clear `hudMessage_`。或保留行為但加 `HudExpired()` accessor 給 harness 用。
- **預期工作量**: ~5 行 + test。

---

## 附錄 A — fact-check 後的 BUGLEDGER 修正建議

- **STRICT_REVIEW / R3 / ROADMAP** prose 雖未細查,但下面是新發現的「已實作但文件沒寫清」:
  - **outdated**: `interlude_market.md` 仍然描述「公告板 NPC」對話選項,實作早已是 trigger-zone(`InterludeExit.h`)。 → BUGLEDGER 應新增「Stale-doc cross-ref」條目。
  - **outdated**: `chapter4.md` 補2 / 補4 「Ch4 傘架場景」、「Flag_ScoldedSenior 學長不出場」標為 KNOWN OMISSION,但設計層 BUGLEDGER 沒對應條目;cycle8-audit 已標但未轉到 BUGLEDGER → 建議補一條 [OPEN] 條目。
  - **verified**: `main.cpp` 是 ~60 行的薄 composition root(`wc -l src/main.cpp` 可確認),不是舊「monolith」描述 — CLAUDE.md §5 寫的 53→~60 line 仍正確。
  - **verified**: H1 EventBus RAII unsubscribe 已實作(BUGLEDGER H1 [FIXED]),playtest 沒看到 UAF 跡象。
- **新發現的 unsubscribed event**:`EventType::KarmaChanged` 有 1 個 publisher (CursedUmbrella.cpp:17) 但 0 個 subscriber → BUGLEDGER 應新增「[OPEN] Dead event channel: KarmaChanged」。

## 附錄 B — Determinism check
- 重跑同一 script 已驗證 byte-identical(per CLAUDE.md §4 / harness 文件)。本次三個 script 各跑一次;若需再驗,可 `diff <(.claude/tools/playtest.sh A run1) <(... A run2)` 在 state.jsonl。**不在本次 read-only 任務範圍**。

---
*產出: cycle9 起手 read-only 診斷;不修 src/;次輪 specialist 從上述 H1-L9 候選照單修。*
