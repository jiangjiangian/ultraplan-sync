# Cycle 9.F — Post-iteration Diagnosis at 401707d

## Executive Summary
- Regression-flagged: 2 (chapter-clear toast clobbered; L8 1-frame respawn lag unchanged).
- New UX gaps: 5 (single-slot toast, IL marker not at shot cadence, `!!` tier unreachable, cold-senior uncovered, IL-exit-hint collision).
- Determinism: PASS. A == A_rep byte-identical; RM == A (View-only); LG diverges as designed.
- Env-vars (M2/M3): PASS. M2 enlarges talk reach (~3 px earlier interact); M3 disables phase/fade in View.

| Run | md5 | lines | end |
|---|---|---|---|
| cycle9f_diag_a / _rep / _rm | 914345b8…22054 | 7490 | A |
| cycle9f_diag_a_lg (LARGE_TARGETS=1) | 953e1f37…fa09e | 7450 | A |
| cycle9f_diag_b / _c | — | 4834 / 4142 | B / C |

## A. H4 quest-giver `!` indicator review
Visually validated across endings A/B/C. Frame 0 (Ch1, all endings): one tiny gold pixel above `victim` — small, off-sprite, no occlusion of dialog box or building name. Frame_004200 Ch3 entry (ending A): the 3 indicators above `vendor_sausage_a` (x=760), `loudspeaker_b` (x=1320), `senior_c` (x=1080) are visible clustered along the south corridor — at the 16-px panel size + 20 px lift they appear as 3 spaced gold dots, no overlap, no occluding the player. No jitter detected: ChapterSpawns y=1820/1825 (static, not wandered) — these NPCs don't move so the indicator sits still. Confirms 9.B H4 fix landed cleanly. **No regression.**

## B. Chapter toast timing — **CRITICAL CLOBBER**
The 9.A.2 chapter toasts use a single-slot HUD (World::SetHudMessage), not a queue. At every Ch→IL transition the chapter-clear toast (`✓ 章節清關 — 進入幕間市集`) and the IL arrival hint (`市集中央。逛完後往南離開`) publish on the same / adjacent frame. Result: the clear toast lives 0.02 s (1 frame) before being overwritten. Repro from ending_a frames 1765 / 3690 / 5084 (3/3) — also `✓ 抵達結局` lives 0.47 s only because the run ends. Ending B/C identical pattern.

Root cause: GameController.cpp:154-163 publishes `kInterludeArrivalHint` right after the chapter toast; the comment claims "the player sees BOTH" but `World::SetHudMessage` (include/World.h:134) overwrites every publish. **The H2 chapter-toast feature is effectively unobservable on chapter clears.** A FIFO or ~2-second delay on the arrival hint would fix it.

## C. karma toast × chapter toast collisions
21 close (<3 s) ShowMessage pairs in A, 15 in B, 17 in C. The 3 chapter-clear collisions in (B) dominate; `業力 +10 → ✓ 抵達結局` clobbers at fr 7454→7462 (0.13 s). The (B) fix subsumes these.

## D. H3 Interlude ground marker visibility
Marker drawn at world y=`kInterludeExitMinY` (=1900). IL window spans ~345 frames per visit; at SHOT_EVERY=600 we get at most one IL shot per visit (frame 1800 captured, player at y=1497 — far north, marker off-camera below). The marker IS drawn (test coverage exists) but **the diagnostic shot cadence cannot validate it visually**. Recommend a tighter SHOT_EVERY (60) for IL probes or a dedicated `il_marker_shots.txt` script driving the player to y~1880.

## E. rain HUD prefix verification
Ending A max rain 57.2%, ending B max 49.6%, ending C max 80.8% (1452 frames in the warning tier `60 ≤ rain < 85`). The `!!` critical tier (`rain ≥ 85`) is **never reached** in any of the three production scripts. So while `RainTierPrefix` (include/RainHud.h:18) returns `" !"` correctly for C, the `"!!"` branch is exercised only by `test_rain_hud.cpp` — no harness-level visual evidence exists. Padding (the leading 2 chars) renders correctly in screenshots (rain readout is left-aligned consistently across all shots inspected). **Recommend a rain-cap probe script** that hits ≥85% so the `!!` glyph is visible in a captured frame.

## F. Ch4 cold-senior path uncovered
`Flag_ScoldedSenior` is used in 4 places (`DialogOpener.cpp:101`, `:181`, `:211`; `World.cpp:113-120` for the M7 ripple). None of `.claude/scripts/ending_{a,b,c}.txt` choose the (c) 不認識 option that sets it. Final flag set in all three endings excludes it. **The 9.C M7 fix has no regression script.** Recommend `.claude/scripts/probe_cold_senior.txt` that picks (c), then runs into Ch4 and asserts `Flag_Ch4Rippled_Senior` did NOT fire and `suit_senior` is absent from the Ch4 roster.

## G. New problems found (8-12 shot sweep)
1. **L8 1-frame respawn lag confirmed unchanged.** At every transition frame N: new SemesterState shown but OLD chapter's npcs[] still listed; on N+1 the new roster appears. Repro across all 7 transitions in ending A. Not fixed in 9.A.2.
2. **IL `npcs[]=[]` empty.** Vendors have empty `NpcId()` (Vendor.cpp:46-48). Harness scripts can only `interact <label> <x> <y>` by position. Not a regression — a usability hole in the DSL.
3. **Quest-giver `!` is sticky past quest completion.** `IsQuestGiver()` (include/NPC.h:52) is a static bool, not "has-pending-quest". `victim` keeps the gold `!` after Flag_PromisedVictim is set.
4. **Pickup `ShowMessage` (`你撿到了 TrueUmbrella，雨停了。`) is also clobbered.** Same frame as chapter toast at fr 1765 / 3690 / 5084. The pickup message never visibly appears — the H2 fix's same-frame publish races it.
5. **`MaybeAnnounceInterludeExit` fires alongside next chapter toast.** Ending A fr 2110: `準備離開市集` and `✓ 進入第二章 期中考` both publish — leaving-hint lives 0 frames. Same single-slot clobber.

## H. Sundry observations
- All 3 endings: 8 transitions, no soft-locks, no crashes (rc=0 across 5 runs).
- `Flag_ConsoledTA` set in both A and B — gate differentiation works correctly.
- `Flag_Ch4Rippled_*` only in A — M7 closure fires on the "all helped" branch only.
- L9 toast TTL: non-colliding toasts live 4.00–4.13 s — kHudTtl honoured.

## I. Cycle 9.G+ candidate work items
| Priority | Item | Entry file:line | Rough effort |
|---|---|---|---|
| H | Queue / delay chapter-toast vs IL-arrival hint (don't clobber) | src/GameController.cpp:154-163, include/World.h:134-140 | medium — add deferred FIFO or 60-frame delay token |
| H | Pickup ShowMessage (`TrueUmbrella`) vs chapter toast race | src/TrueUmbrella.cpp / UmbrellaPickup chain + same World HUD slot | small — re-order publishes OR same queue as above |
| M | IL exit hint vs next chapter toast (same single-slot clobber) | src/GameController.cpp (`MaybeAnnounceInterludeExit`), src/ChapterToast.h:79 | small (folded into H queue fix) |
| M | Cold-senior regression script + assertion | new .claude/scripts/probe_cold_senior.txt | small — author 60–100-line script that picks (c) |
| M | Rain-cap probe script to hit `!!` tier | new .claude/scripts/probe_rain_cap.txt | small — drive player without umbrella through 4 chapters |
| L | Tighter SHOT_EVERY for IL diagnosis | .claude/tools/playtest.sh (no change) or a wrapper | trivial — just call with a smaller arg |
| L | Quest-giver `!` clears after quest done | include/NPC.h:52 + World scan of relevant flags | medium — needs per-id flag map |
| L | L8 1-frame respawn lag | src/GameController.cpp Transition handler + Harness.cpp:181 | medium — defer state-flip OR pre-spawn before publishing |
| L | Vendor NpcId for IL harness driveability | src/Vendor.cpp:46-48 | small — add `vendorId` arg, plumb through factories |

## J. Determinism / flag-verification appendix
```
914345b8120918385a2738ec25522054  cycle9f_diag_a/state.jsonl
914345b8120918385a2738ec25522054  cycle9f_diag_a_rep/state.jsonl   ← PASS, byte-identical
914345b8120918385a2738ec25522054  cycle9f_diag_a_rm/state.jsonl    ← PASS, RM is view-only (expected eq)
953e1f37b12d4efa7bb5e650088fa09e  cycle9f_diag_a_lg/state.jsonl    ← differs (intended: larger talk reach)
```

LG divergence at frame 893 (first `interact ta`): A at x=1739 dialog OFF, LG at x=1742 dialog ALREADY ACTIVE (16 px reach vs 8 px). LG run 40 frames shorter, same ending A, same 19 terminal flags.

RM byte-identical because `ReducedMotion` flips only View-side helpers (InterludeMarkerPhaseStep / EndingFadeAlphaStep / HudToastFadeT). Visual validation needs multi-shot animation scan, not single frames.

L8 transition inspection (cycle9f_diag_a, 7/7 transitions affected): on N the semester has flipped but npcs[] is the OLD chapter's; on N+1 the new roster appears. Confirmed: 1765 (Ch1→IL keeps Ch1 NPCs), 2110 (IL→Ch2 empty), 4014 (IL→Ch3 empty), 5430 (IL→Ch4 empty), 7462 (Ch4→End A keeps Ch4 NPCs).
