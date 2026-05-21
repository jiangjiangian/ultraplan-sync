# Audit — docs/content/chapter1.md

**Overview (≤3 sentences):** Ch1「加退選之亂」is the spine opener and is well-wired: NPC roster (`NpcSpawns.h:29-50`), four umbrellas + form pickup (`World.cpp:29-37`), karma deltas, all `Flag_*` writes/reads, the chapter gate, and rain↔respawn loop are all implemented in `src/`. Cycle-8 just re-authored 學長 (b) from inert decline → 「斥責」 lighting up the long-dead `Flag_ScoldedSenior` arc; the (c) note on 阿姨 still calls out the post-Cycle-8 removal of `Flag_KnowsUglyUmbrella` (dead-annotation cleanup per B3 precedent). Two doc-only stylings remain (傘架 prop & 苦主 cold-walk auto-trigger) but no logic conflicts with src/.

## Per-element annotations

- **SemesterState `Chapter1_AddDrop`** — initial chapter state. **[是否實作?]** Yes — `include/SemesterStateMachine.h:11,34`. **[邏輯衝突?]** No.
- **Starting `rainMeter = 0`** — fresh-start state. **[是否實作?]** Yes — `src/Player.cpp:34` (`rainMeter_(0.0f)`). **[邏輯衝突?]** No.
- **Starting `karma = 50`** — neutral 50 baseline. **[是否實作?]** Yes — `src/Player.cpp:34` (`karma_(50)`). **[邏輯衝突?]** No.
- **`hasUmbrella = false` initial** — none in hand at spawn. **[是否實作?]** Yes — `src/Player.cpp:34` (`hasUmbrella_(false)`). **[邏輯衝突?]** No.
- **累積型 karma (Δ×0.6, ending 80/0/N gates)** — small deltas, not single-shot. **[是否實作?]** Partial — karma additive & clamp `[-100,100]` (`src/Player.cpp:97-98`); per-delta scaling NOT applied (no `×0.6` multiplier — doc-only narrative claim). **[邏輯衝突?]** No (intentional: GDD §伍 baseline; CHANGELOG Cycle-3 F3a re-confirms gate math).
- **Clear condition: `TrueUmbrella` claimed** — chapter advances on True claim. **[是否實作?]** Yes — `include/EventWiring.h:44-55`. **[邏輯衝突?]** No.
- **Clear condition: 「持有任意傘種離開集英樓」** — alt location-based clear. **[是否實作?]** No — `EventWiring.h:40` explicitly defers ("Phase 2"). **[邏輯衝突?]** No (intentional defer, doc-acknowledged).
- **玩家起點: 綜合院館 1 樓大廳** — narrative start point. **[是否實作?]** Partial — `Player::RespawnAtGate` lands at 正門 `{500,1860}` (`src/Player.cpp:154`); the doc itself notes "正門附近的動線屬旁白補充". **[邏輯衝突?]** No (doc-flagged narrative-vs-impl reconciliation).
- **rainMeter ≥ 50 系統訊息 (speed penalty)** — slow + soak. **[是否實作?]** Partial — accrual lethal-gate exists (`src/Player.cpp:119-127`) but NO movement speed penalty in `Player`/`GameController`. **[邏輯衝突?]** No — CHANGELOG Cycle-3 I8 explicitly DEFERRED speed penalty.
- **rainMeter ≥ 100 強制傳送 + 半天 + 限時錯過** — lethal teleport. **[是否實作?]** Partial — teleport + reset implemented (`src/Player.cpp:124-125,152-159`), 半天 logged as `ShowMessage` text only (no in-game clock); "限時情報永久錯過" — not modelled. **[邏輯衝突?]** No (CHANGELOG Cycle-4 activated lethal rain; clock not modelled).
- **NPC 西裝學長 `isQuestGiver=true`** — quest opener. **[是否實作?]** No — `NpcSpawns.h:36-37` sets `isQuestGiver=false`. **[邏輯衝突?]** No (intentional; only 苦主 is Ch1 quest-giver, see `NpcSpawns.h:32`). Doc-stale annotation.
- **學長 (a) 初次接觸 dialogLines** — opener menu. **[是否實作?]** Yes — content-loaded via `DialogLoader` + `DialogOpener.cpp:99-103` opener routing. **[邏輯衝突?]** No.
- **學長 (b) 斥責, `karma -5`, `Flag_ScoldedSenior=true`** — Cycle-8 redesigned beat. **[是否實作?]** Yes — read at `src/DialogOpener.cpp:101`, `src/Chapter2Quest.cpp:66`; set via DialogChoice mechanism. **[邏輯衝突?]** No (CHANGELOG 2026-05-20 Cycle-8 F2; intentional re-author over inert peaceful-decline).
- **學長 (b) note: ProfTrap not spawned on this path** — narrative consistency. **[是否實作?]** Partial — ProfessorTrapUmbrella always spawns at `{1200,1256}` (`src/World.cpp:31`), regardless of player path. **[邏輯衝突?]** No (doc-flagged: TrueUmbrella shared at `{320,1280}`; the trap stays unclaimed if not led to by suit_senior — narrative-only claim).
- **學長 (c) 接受跑腿, `karma -5`, ProfTrap spawn** — accept-bring-trap. **[是否實作?]** Partial — ProfTrap pre-spawned in World; `Flag_HasProfessorTrap` set on claim (`src/ProfessorTrapUmbrella.cpp:13`); the (c) DialogChoice still routes via DialogLoader/Opener once-apply. **[邏輯衝突?]** No.
- **學長 (d) 點破, `karma +3`, `Flag_HelpedSenior=true`** — good-path. **[是否實作?]** Yes — flag read across `src/DialogOpener.cpp:100,184,216`, `src/Chapter4Quest.cpp:14`; karma applied via opener once-apply. **[邏輯衝突?]** No.
- **NPC 學霸 `isQuestGiver=false`** — info-only. **[是否實作?]** Yes — `NpcSpawns.h:40-41`. **[邏輯衝突?]** No.
- **學霸 (b) `karma +3`** — virtuous-listener bonus. **[是否實作?]** Yes — content-loaded; opener once-apply path. **[邏輯衝突?]** No.
- **NPC 助教 `isQuestGiver=true`** — quest beat. **[是否實作?]** No — `NpcSpawns.h:44-45` `isQuestGiver=false`. **[邏輯衝突?]** No (stale doc annotation; behavior carried by `Flag_FoundForm`/`Flag_HelpedTA_Ch1` opener routing — `DialogOpener.cpp:77-78`).
- **助教 (b) form quest, `karma +5`, `Flag_HelpedTA_Ch1=true`** — form pickup reward. **[是否實作?]** Yes — `QuestFlagPickup` at `{560,1725}` sets `Flag_FoundForm` (`World.cpp:36-37`); opener routes (b) once flag set; Ch4 ripple at `Chapter4Quest.cpp:44`. **[邏輯衝突?]** No.
- **助教 (c) `karma -15` + 追逐狀態** — caught with ProfTrap. **[是否實作?]** Partial — `Flag_HasProfessorTrap` triggers (c) Ch2 -10 ripple (`Chapter2Quest.cpp:66`); Ch1 -15 punishment via opener once-apply on the (c) substate (DialogChoice in content). Chase-state NPC behavior NOT implemented (no AI state machine). **[邏輯衝突?]** No (chase is narrative flavour; karma penalty lands).
- **NPC 福利社阿姨 `isQuestGiver=false`** — vendor/info. **[是否實作?]** Yes — `NpcSpawns.h:47-48`. **[邏輯衝突?]** No.
- **阿姨 (b) 醜傘 80 元購買** — buy beat. **[是否實作?]** No — Ch1 has NO Vendor; `ChapterVendors.cpp:120-126` returns empty for `Chapter1_AddDrop`. The Ch4 集英樓 Vendor (`ChapterVendors.cpp:108-118`) is the only ugly-umbrella seller (price 100). **[邏輯衝突?]** No — (c) note (lines 209-214) EXPLICITLY documents Ch1 is "narrative seed only"; Ending-C trigger is Ch4 Vendor (`Flag_BoughtUglyUmbrella` read at `EndingGate.cpp:69`).
- **阿姨 (c) `karma +0`, no flag (post-Cycle-8 cleanup)** — neutral seed. **[是否實作?]** Yes — note matches CHANGELOG 2026-05-20 F1; whitelist also stripped (`Harness.cpp` "Flag_KnowsUglyUmbrella" no longer present). **[邏輯衝突?]** No (intentional B3-precedent cleanup).
- **阿姨 (d) 請咖啡 `karma +5`, `Flag_BoughtCoffeeForAuntie_Ch1=true`** — generosity ripple. **[是否實作?]** Yes — `Chapter4Quest.cpp:33` reads `kFlagBoughtCoffeeForAuntie` (= `"Flag_BoughtCoffeeForAuntie_Ch1"`, `Chapter4Quest.h:29-30`); `DialogOpener.cpp:251` routes Ch4 阿姨 to (a) direct-info. **[邏輯衝突?]** No (Cycle-1 B3 fix).
- **NPC 苦主 `isQuestGiver=true`** — Ch1 lead-quest-giver. **[是否實作?]** Yes — `NpcSpawns.h:31-32`. **[邏輯衝突?]** No.
- **苦主 (b) `karma +5`, `Flag_PromisedVictim=true`** — promise. **[是否實作?]** Yes — `DialogOpener.cpp:82,142,175`; also gates `TransparentUmbrella` claim (`TransparentUmbrella.cpp:15`). **[邏輯衝突?]** No.
- **苦主 (c) 無視走過 `karma -3`** — cold-walk auto-trigger. **[是否實作?]** No — there is no proximity/exit trigger in `GameController` or `World` that fires when the player leaves the entrance area without talking. **[邏輯衝突?]** No (narrative-doc-only; no flag is referenced anywhere downstream, so omission is inert).
- **TrueUmbrella claim → 章節清關 → Interlude** — main path. **[是否實作?]** Yes — `EventWiring.h:44-55`; spawned at `World.cpp:29` `{320,1280}`. **[邏輯衝突?]** No.
- **ProfessorTrapUmbrella `Flag_HasProfessorTrap` + Ch2 ripple -10** — ripple seed. **[是否實作?]** Yes — `ProfessorTrapUmbrella.cpp:13`; `Chapter2Quest.cpp` ripple table. **[邏輯衝突?]** No.
- **FragileUmbrella 20% rate residual rain** — broken-but-clear. **[是否實作?]** Partial — `FragileUmbrella` spawns (`World.cpp:30`) and sets `hasUmbrella_=true` via parent; the residual 20% rate is NOT differentiated — `ApplyRainSheltered` is +1.5 u/s ≈ 30% (CHANGELOG #5). **[邏輯衝突?]** No (CHANGELOG #5 made the rate universal across umbrella types — intentional design refactor; chapter1.md's "20%" is doc-stale relative to current 30% global rate but not behavior-changing).
- **CursedUmbrella `karma -30` + `Flag_TookCursedUmbrella`** — Ending B seed. **[是否實作?]** Yes — `CursedUmbrella.cpp:13-14`, `CursedUmbrella.h:13` (`karmaPenalty_(30)`); `EndingGate.cpp:49`. **[邏輯衝突?]** No (Cycle-3 F2 rebalance 50→30 documented in CHANGELOG).
- **「都沒拿」rainMeter 100% 強制傳送 + 不扣 karma + 可重試** — retry loop. **[是否實作?]** Partial — `RespawnAtGate` runs (no karma loss confirmed by `Player.cpp:152-159`); the "連續三次提示" hint is NOT implemented. **[邏輯衝突?]** No (hint is doc-aspirational; core teleport is wired).

## Summary
- **Implemented (Yes):** 16
- **Partial:** 9
- **Not-implemented (No):** 4 (學長/助教 `isQuestGiver=true` stale; 阿姨 (b) 80元購買 — Ch1 no Vendor; 苦主 (c) auto-trigger)
- **Conflicts:** 0 (every Partial/No either traces to a CHANGELOG-acknowledged intentional defer, or to doc-flagged narrative-only flavour)
- **Stale-doc-only:** 4 (累積型 ×0.6 multiplier; 學長/助教 isQuestGiver markers; Fragile 20% rate vs 30% global)
