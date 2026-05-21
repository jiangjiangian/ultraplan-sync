# Audit — docs/content/chapter4.md

**Overview (≤3):**
1. Ch4 narrative spine (5 NPCs × ripple flags, finale 體諒/質問 choice, 3 ending triggers) is solidly implemented: `EndingGate.cpp` now TOTAL per Cycle-6 L1, `DialogOpener.cpp:285-313` finale, `Chapter4Quest.cpp` ripples, `EndingGate.cpp:47-49` cold-finale→B.
2. Chapter-metadata mechanics promised in L7 are mostly fiction: `視線壓縮` (環境光 -30%, rain accrual ×1.5, 地圖全開) has **no implementation** in `src/GameController.cpp`, `View.cpp` or any chapter table — flat Ch4 rain rate = Ch1's +5 u/s.
3. Starting state partly implemented: `hasUmbrella=false` is wired (`GameController.cpp:153`), but `rainMeter=30` Ch4 entry is **not** (`Player::rainMeter_` starts 0 per `Player.cpp:34`, never re-seeded on Ch4 entry); `karma 沿用` is true by default (no reset).

## Per-element annotations

- **Metadata: `SemesterState: Chapter4_Finals`** — engine state.
  - [是否實作?] **Yes** — `include/SemesterState.h:12`, `include/Chapter4Finals.h:9`, `src/SemesterStateMachine.cpp:56`.
  - [邏輯衝突?] **No**.

- **karma 沿用 Ch3 結算值（×0.6 累積型，不重置）** — karma carries; non-linear "×0.6" multiplier.
  - [是否實作?] **Partial** — non-reset Yes (`Player` never resets `karma_`); "×0.6 累積型" multiplier has no implementation — additions are linear `AddKarma` calls.
  - [邏輯衝突?] **Yes** — INTENTIONAL: Cycle-8 N1, Cycle-3 F2 reaffirm `-15/-30` linear scale; "×0.6" is GDD prose retained as flavour. Treat as historical (CLAUDE.md §2 tier-4).

- **rainMeter = 30 起始（磅礡大雨）** — Ch4 starts at 30%.
  - [是否實作?] **No** — `Player.cpp:34` initialises to 0, no Ch4 re-seed in `GameController.cpp:153`.
  - [邏輯衝突?] **Yes (REAL gap)** — the documented "氣壓壓下來" feel is absent; not in CHANGELOG.

- **hasUmbrella = false 起始** — player loses umbrella on Ch4 entry.
  - [是否實作?] **Yes** — `GameController.cpp:153-158` resets `SetHasUmbrella(false)` + `ClearFlag("Flag_HasTrueUmbrella")` on Ch4 enter.
  - [邏輯衝突?] **No**.

- **新機制：視線壓縮（環境光-30%, rain ×1.5, 地圖全開）**
  - [是否實作?] **No** — no Ch4 light dim in `View.cpp`; rain rate flat at +5 u/s for all chapters (`Player.cpp:123`); no "地圖全開" gate logic.
  - [邏輯衝突?] **Yes (REAL gap)** — documented unique Ch4 atmosphere is fiction; could be INTENTIONAL trim (the assignment doesn't require it) but undocumented.

- **死亡傳送 rainMeter ≥ 100 → 正門** — lethal rain.
  - [是否實作?] **Yes** — `Player.cpp:124-125, 152` `RespawnAtGate()`; Cycle-4 lethal wired.
  - [邏輯衝突?] **No**.

- **場景旁白 (開場 / 系統訊息 / 章節推進字卡)** — narrator beats.
  - [是否實作?] **N/A** — runtime-loaded by `DialogLoader.cpp`; `dialog_lint` passes (0 err/0 warn); these don't gate anything.
  - [邏輯衝突?] **No**.

- **NPC 西裝學長 (a) 假笑面具** — opener iff `Flag_HelpedSenior`.
  - [是否實作?] **Yes** — `DialogOpener.cpp:100`, `Chapter4Quest.cpp:14` ripple.
  - [邏輯衝突?] **No**.

- **西裝學長 (b) karma>70 崩潰坦白 +10** — peak ripple.
  - [是否實作?] **Partial** — `Chapter4Quest.cpp:13-15` lands +10 once iff `HelpedSenior && karma>70`; opener routing has only HelpedSenior check (`DialogOpener.cpp:100`), so the karma>70 GATE is enforced only by the ripple, not the substate routing — (b) substate may show even at karma 50-70.
  - [邏輯衝突?] **Yes (minor)** — REAL: routing/karma-gate decoupled.

- **西裝學長 (c) karma<30 翻臉** — half-info, karma 0.
  - [是否實作?] **Partial** — `DialogOpener.cpp:101` routes (c) on `Flag_ScoldedSenior` (Cycle-8 N1), not on `karma<30`. The doc-stated "karma<30" gate is NOT what code does.
  - [邏輯衝突?] **Yes** — REAL drift: doc says karma threshold, code uses ScoldedSenior flag.

- **西裝學長 (d) 道德考驗結算** — karma-tiered farewell.
  - [是否實作?] **N/A** — pure flavour lines; no flag/karma side-effects to gate.
  - [邏輯衝突?] **No**.

- **NPC 學霸 (a)/(b)/(c) Flag_BookwormRecovered 分支** — Ch2 ripple.
  - [是否實作?] **Yes** — `DialogOpener.cpp` BookwormRecovered routing; `Chapter4Quest.cpp:19-23` +5 karma once.
  - [邏輯衝突?] **No**.

- **學霸 (d) 結算** — karma-tiered.
  - [是否實作?] **N/A** — flavour only.
  - [邏輯衝突?] **No**.

- **NPC 助教 (a)/(b)/(c) HelpedTA_Ch1 / HasProfessorTrap 分支** — peak ripple.
  - [是否實作?] **Yes** — `DialogOpener.cpp:107-108`, `Chapter4Quest.cpp:38-49` independent +10 / -15 callbacks (precedence honoured).
  - [邏輯衝突?] **No**.

- **助教 (d) 體諒/質問 結算 +15 / -5** — Ending A gate.
  - [是否實作?] **Yes** — `DialogOpener.cpp:296-308` hard-codes both choices; `EndingGate.cpp:24-29` reads `Flag_ConsoledTA`. Cycle-6 L1 made gate TOTAL (no soft-lock).
  - [邏輯衝突?] **No**.

- **NPC 福利社阿姨 (a) 直接情報 / (d) 間接情報** — Coffee ripple.
  - [是否實作?] **Yes** — `DialogOpener.cpp` routes on `Flag_BoughtCoffeeForAuntie_Ch1`; `Chapter4Quest.cpp:27-35` +3 once (B3 fix).
  - [邏輯衝突?] **No**.

- **福利社阿姨 (b) 推銷綠傘 / (c) 拒買** — Ending C signpost.
  - [是否實作?] **Partial** — substate exists & loads; aunt is not the Vendor — `ChapterVendors.cpp:108-114` (集英樓) is. Doc explicitly says "指路人" so this is INTENTIONAL.
  - [邏輯衝突?] **No** (matches §409 補設定 6).

- **NPC 苦主 (a) 釋懷 / (b) 淡漠** — Promise ripple.
  - [是否實作?] **Yes** — `DialogOpener.cpp:142, 173-175` Ch4 routing on `Flag_PromisedVictim`.
  - [邏輯衝突?] **No**.

- **苦主 ending-A 出場 (Flag_PromisedVictim && 傘已還)** — `chapter4.md:356`.
  - [是否實作?] **No** — Ending A roster `kEndingA` empty (`ChapterSpawns.h:96`, TODO).
  - [邏輯衝突?] **Yes** — REAL gap, INTENTIONAL deferred (S5e TODO marker).

- **Ending A 觸發 karma>80 + TrueUmbrella + ConsoledTA + Ch1~Ch3 全主線** — A trigger.
  - [是否實作?] **Partial** — `EndingGate.cpp:27-29` checks karma>80 + `Flag_HasTrueUmbrella` + `Flag_ConsoledTA`; "Ch1~Ch3 全主線" not gated (no `Flag_Ch1/2/3Cleared` aggregate check).
  - [邏輯衝突?] **No** — INTENTIONAL: pre-Ch4 chapter advancement is implicit in reaching Ch4.

- **Ending B 觸發 karma<0 OR CursedUmbrella OR Ch4 傘架取他人傘** — B trigger.
  - [是否實作?] **Partial** — `EndingGate.cpp:47-50` covers karma<0, `Flag_TookCursedUmbrella`, plus cold-finale (extension); the new "Ch4 傘架場景" (§407 補設定 4) has no implementation.
  - [邏輯衝突?] **Yes** — REAL gap: doc 補設定 4 says "Ch4 傘架場景" added, but no such scene exists.

- **Ending C 觸發 集英樓便利商店買綠傘 + 花光錢** — C trigger.
  - [是否實作?] **Partial** — `ChapterVendors.cpp:111-114` 集英樓 sells UglyUmbrella → `Flag_BoughtUglyUmbrella`; "花光所有遊戲幣" is NOT enforced (`ChapterVendors.cpp:104` calls it "flavour, not mechanic"); price 100, soft-cap 300, no all-money confirm prompt.
  - [邏輯衝突?] **Yes** — REAL: "花光" is doc-only.

- **章節推進字卡 (3 拍)** — narrator inserts.
  - [是否實作?] **N/A** — narrator text, not gated logic.

- **補設定 1-7 (作者標注的補設定)** — design notes.
  - [是否實作?] 補2 (學長`Flag_ScoldedSenior` 不出場): **No** — `ChapterSpawns.h:81-93` always spawns suit_senior (commented as "KNOWN OMISSION"). 補4 (Ch4 傘架): **No**. 補5 (苦主固定正門廊柱): **Yes** — `ChapterSpawns.h` victim x=380,y=1860. 補7 (二次購買): **Yes** — Vendor `qty=-1` infinite (`ChapterVendors.cpp:113`).
  - [邏輯衝突?] **Yes (補2/補4 — REAL gaps, INTENTIONAL deferral per ChapterSpawns.h comment)**.

## Summary
- **Elements audited:** 27
- **Yes:** 11 · **Partial:** 7 · **No:** 5 · **N/A:** 4
- **Conflicts cited:** 9 (rainMeter=30 seed; 視線壓縮 mechanic trio; karma ×0.6; 學長(b) karma>70 routing decoupled; 學長(c) ScoldedSenior vs karma<30; 苦主 Ending-A roster; Ending-B 傘架 scene; Ending-C 花光錢; 補2/補4 deferred). Of these, 4 are INTENTIONAL Cycle-N redesigns (karma linear, "花光" flavour, 補2/補4 noted-as-omitted), 5 are REAL undocumented gaps (rainMeter 30 entry seed, 視線壓縮 trio, suit_senior (b)/(c) routing-vs-doc drift, Ending-A 苦主 roster TODO).
