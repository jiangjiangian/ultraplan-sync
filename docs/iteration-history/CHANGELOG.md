# CHANGELOG — 《尋傘記》 self-iteration

Every code/design change: what · why · gameplay impact · revert. Newest
first. Bugs cross-reference `.claude/BUGLEDGER.md`.

---

## 2026-05-21 — Cycle 9.B: feedback signal closure — quest-giver indicator + karma toasts + chapter-clear visibility

Closes the remaining items the 9.A.2 commit explicitly deferred and
caps the "player feedback" theme. Three parallel small fixes, one
new visual primitive.

### What
- **H4 — NPC `!` quest-giver indicator.** New header
  `include/QuestGiverIndicator.h` exposes
  `LayoutQuestGiverIndicator(gfx::Rect hitBox)` (pure geometry) +
  `DrawQuestGiverIndicator(gfx::IRenderer&, gfx::Rect)` (rendering
  through the engine-agnostic `IRenderer` interface). View.cpp
  now calls it inside the `CameraScope` for every `GameObject` whose
  newly-virtualised `IsQuestGiver()` returns `true`. Layout: 16×16
  gold `#FFC83D` square + black `!` glyph + 2 px translucent drop
  shadow, floating 20 px above the NPC sprite top. Previously
  `View.cpp` ignored `NPC::IsQuestGiver()` entirely — the flag was
  set on Victim, Chapter-3 trade chain, and the Ch4 finale NPCs but
  the player got no visual hint where the next interaction lived.
- **TrueUmbrella publish-order swap.** `src/TrueUmbrella.cpp`
  switches the two `Publish()` lines so `ShowMessage("你撿到了
  TrueUmbrella")` fires *before* `UmbrellaClaimed`. The 9.A.2
  H2 `UmbrellaClaimed` subscriber writes the chapter-clear toast
  on top of the umbrella message and is now the persistent HUD
  text at frames 1765 (Ch1→Interlude) and 5084 (Ch3→Interlude). Net
  player-visible HUD on the pickup-path transitions:
  `✓ 章節清關 — 進入幕間市集` (was: `你撿到了 TrueUmbrella`).
  Frame 5835 (Ch4 reclaim, no chapter transition) still shows the
  umbrella text — pickup outside a transition is unchanged.
- **H5 — karma toasts via `KarmaChanged` channel.**
  `Player::AddKarma(int delta)` now publishes
  `Event{KarmaChanged, "%+d"-formatted delta}` on every non-zero
  delta (the 0-delta gate keeps `+0` / `-0` banners off the HUD).
  A new `WireKarmaToastSubscriber()` in `include/EventWiring.h`,
  wired by `GameController` right after `WireHudMessageSubscriber`,
  re-publishes the delta as a `ShowMessage` with `業力 ` prefix —
  so the existing HUD subscriber paints it for free. The dead
  hand-rolled `KarmaChanged` publish in `CursedUmbrella::beClaimed`
  removed: it duplicated what now flows through the unified
  `AddKarma` path. Plan A from the brief (single HUD slot, chapter
  toast wins simultaneous collisions); a future cycle can promote
  karma to a dedicated channel if playtesting shows the toast too
  easy to miss.
- **L9 — HUD message expiry.** `World::HudExpired()` returns
  `hudAge_ >= kHudTtl && !hudMessage_.empty()` — a read-only
  predicate, NOT a clear. `MessageView`'s fade-out animation
  contract (which reads both `hudMessage_` and `hudAge_`) stays
  byte-identical. `Harness.cpp:168` checks `HudExpired()` and emits
  `""` for the `hud` field in `state.jsonl`, so the snapshot stream
  no longer echoes aged-out toasts forever. 35% of ending_a's
  7,490 frames now record empty HUD; before this, every frame
  inherited whatever the last `SetHudMessage` had said.

### Why
9.A.2 closed three feedback gaps but left two visible:
(a) at the Ch1→Interlude and Ch3→Interlude pickup paths the H2
chapter-clear banner was getting overwritten by the second
TrueUmbrella publish in ~0 frames (commit notes flagged this);
(b) the diagnosis listed H4 (`!` indicator), H5 (karma toast) and
L9 (HUD reset) as the natural finishers of the "every gameplay
signal becomes visible HUD text" arc, with mechanical 5–25-line
fixes already scoped. 9.B does all four in one parallel pair of
agent passes — gameplay-quest-designer (TrueUmbrella + H5 + L9)
and raylib-gfx-uxui (H4).

### Gameplay impact
- 318/318 doctest cases pass (was 305 → +13 cases, 4424 assertions).
- 0 project-code warnings under `-Wall -Wextra -Wpedantic`.
- Three endings still byte-clean: ending_a smoke
  (frames=7,490, last_hud=`✓ 抵達結局`, last_semester=結局 A,
  karma_toasts_seen=14, empty_hud_frames=2623/7490 ≈ 35%);
  ending_b smoke (frames=4,834, last_semester=結局 B);
  ending_c smoke (frames=4,142, last_semester=結局 C). State.jsonl
  is no longer byte-identical to pre-9.B snapshots — that's the
  feature, not a regression. Re-running the same script against
  this HEAD remains byte-deterministic.
- Determinism note: bus dispatch is single-threaded in-order; no
  new RNG, no new threading.

### Revert
- `git rm include/QuestGiverIndicator.h tests/test_quest_giver_indicator.cpp tests/test_karma_toast.cpp tests/test_hud_reset.cpp`
- Revert the publish-order swap in `src/TrueUmbrella.cpp`
- Drop `Player::AddKarma`'s new publish + restore the
  `CursedUmbrella` hand-rolled publish
- Remove `World::HudExpired()` and the Harness branch on it
- Re-virtualise `GameObject::IsQuestGiver()` back to non-virtual /
  inherited behaviour
The two paths roll back independently — visual half and logic half
do not share files.

### Test-suite gotcha (reminder; matches the 9.A.2 note)
`tests/test_eventbus_isolation.cpp` reporter clears
`EventBus::Instance()` on every `subcase_start/end` +
`test_case_start/end`. New listeners must `Subscribe()` inside each
SUBCASE body, not in the enclosing TEST_CASE. Both new test files
follow this rule and document it at the top.

### Follow-ups still open (NOT in this PR)
- H3 view-side ground marker at y=1900 (Interlude exit visual half;
  raylib-gfx-uxui polish PR; logic half shipped 9.A.2)
- M7 — Ch4 `Flag_ScoldedSenior` skips suit_senior spawn
- L8 — same-frame respawn after Transition (race-audit needed)
- BUGLEDGER B3 — Flag_SawVictim_Ch1 / Flag_BoughtCoffeeForAuntie_Ch1
  ripple-or-remove decision
- KB topic STUBs in `docs/kb/` still empty; specialist agents fill
  on first use per the construction protocol

---

## 2026-05-21 — Cycle 9.A: UX feedback gaps — chapter-transition toasts + Interlude hints

Direct response to user-reported playtest complaints: (1) 找不到關鍵
人物 / 地圖上沒有 NPC, (2) 不知道劇情有沒做對, (3) 沒有任務達成
通知. The bughunter diagnostic (`docs/cycle8-audit/cycle9-ux-diagnosis.md`)
catalogued 6 spawn gaps + 5 discoverability gaps + 5 feedback gaps,
prioritised H1–L9. Cycle 9.A.1 (`7c25bda`) shipped M6 (interlude_market
doc realignment) + non-LLM docs knowledge graph. This cycle (9.A.2)
ships H1+H2+H3.

### What
- **H1 — regression-test net for Ch1→Interlude/Ending NPC sweep.** Three
  new doctest files (`tests/test_world_chapter_roster.cpp` +
  `tests/test_chapter_transitions.cpp` + `tests/test_interlude_exit_feedback.cpp`
  ; +17 SUBCASEs / 4367 assertions). **Diagnosis H1 was wrong**: the
  bughunter inferred Ch1 NPCs leaked because `World.cpp` ctor pushed
  them to `objects_` but not `chapterRoster_`. Reality is `World.cpp:47`
  already calls `RespawnChapterRoster(semester_.Current())` →
  `SpawnChapterNpcs(Chapter1_AddDrop)` → `chapterRoster_.push_back`,
  so the registration was always intact. The observed 5-NPC residue
  in Interlude `state.jsonl` is **L8** (1-frame respawn lag): the
  state log emits AFTER FSM `Transition()` but BEFORE the next-frame
  `GameController::Update` polls. By frame N+1 the NPCs are gone.
  Engine left untouched; regression tests now pin the sweep invariant
  across the full spine (Ch1→Interlude→Ch2→Interlude→Ch3→Interlude→
  Ch4→Ending_A) so a future ctor refactor that DID break tracking
  would fail in CI.
- **H2 — chapter-transition toasts.** New header
  `include/ChapterToast.h` (~85 lines, header-only) declares
  `ChapterTransitionToast(SemesterState)` + `PublishChapterTransitionToast()`.
  Eight Transition sites now publish a `ShowMessage` event with a
  ≤28-cell CJK string ("✓ 進入第二章 期中考",
  "✓ 章節清關 — 進入幕間市集", "✓ 抵達結局", etc): two in
  `EventWiring.h` (Ch1/Ch3 UmbrellaClaimed subscribers), three in
  `src/ChapterGate.cpp`, three in `src/EndingGate.cpp`. Before this,
  state.jsonl recorded five consecutive Transitions with `events=[]`,
  so the HUD only refreshed when a non-transition event happened to
  fire. Pure additive: no existing Publish was changed, only new ones
  appended after each `semester_.Transition(...)` call.
- **H3 — Interlude entry/exit hints.** Two new constants
  (`kInterludeArrivalHint`, `kInterludeExitPrep`) and a
  once-per-visit latch helper `MaybeAnnounceInterludeExit(bool&)`
  live in `ChapterToast.h`. `GameController` got a `bool
  interludeExitArmed_` field + a fresh publish right after
  `RespawnChapterRoster` snaps to `Interlude_Market`, and the latch
  resets on each Interlude arrival so the toast fires exactly once
  per visit even when the player wobbles across the y=1900 boundary.
  View-side ground marker explicitly **deferred** per brief — that's
  a raylib-gfx-uxui follow-up.

### Why
The user's three complaints share a single root cause: every
gameplay signal landed in `state.jsonl.events` and was never lifted
into the HUD. Players never knew the chapter had advanced, never knew
they were "done", never knew where to leave the Interlude. Adding
toast text at the Transition sites is a 1:1 wire fix — the data was
all there; the View just never saw it.

### Gameplay impact
- All three endings still reachable; `ending_a/b/c.txt` smoke runs
  end with `last_state=結局 X`, `last_hud='✓ 抵達結局'`, `last_npcs=[]`.
- Determinism preserved at the byte level for the **deterministic
  inputs** path (same script ⇒ same `state.jsonl` event sequence);
  the HUD text added is itself part of the snapshot, so the file
  is no longer byte-identical to pre-9.A snapshots — but that's the
  intended behaviour change.
- 305/305 tests PASS (was 289/289 → +16 new TEST_CASEs).
- 0 project-code warnings.

### Known follow-up (left intentionally for a polish PR)
- `src/TrueUmbrella.cpp:17-18` publishes `UmbrellaClaimed` then
  `ShowMessage("你撿到了 TrueUmbrella")` in that order. The H2
  chapter-clear toast (published by the `UmbrellaClaimed` subscriber)
  is overwritten in ~0 frames by the second TrueUmbrella message.
  Visible at frame=1765 (Ch1→Interlude) and frame=5084 (Ch3→Interlude).
  Swap the two Publish lines or add a small delay to keep the
  chapter-clear text on-screen longer.
- View-side ground marker for the y=1900 exit band (H3 visual half).
- H4 quest-giver `!` indicator over NPCs (View.cpp ignores
  `NPC::IsQuestGiver()`).
- H5 karma toast (`KarmaChanged` is a dead event channel —
  1 publisher, 0 subscribers).

### Revert
Three test files are additive; deleting them and reverting the
publish lines + `bool interludeExitArmed_` field rolls back cleanly.
`include/ChapterToast.h` would become orphaned — `git rm` it.

### Test gotcha (documented inline in the new tests)
The suite has a `doctest::Reporter` (`tests/test_eventbus_isolation.cpp`)
that calls `EventBus::Instance().Clear()` on every
`subcase_start/end` + `test_case_start/end`. Tests that need a
listener subscribed across SUBCASE bodies must `Subscribe()` **inside
each SUBCASE**, not in the parent `TEST_CASE`. The new tests follow
this rule and document it at the top of each file.

---

## 2026-05-20 — Cycle 8: narrative conformance — dead-flag activation + seed cleanup

Two read-only-audit findings landed (user pre-approved). Both small
content-tier changes; gate green; endings byte-identical.

### What
- **F2 (`496a771`) — activate the GDD §伍 Ch1 斥責 path.** `chapter1.md`
  西裝學長 (b) re-authored from the inert "拒絕，no flag" stub into the
  GDD §伍 選項 B 「憤怒斥責，奪回雨傘」 (karma `-5`,
  `Flag_ScoldedSenior = true`). The flag was previously read by
  `DialogOpener.cpp:101`, `Chapter2Quest.cpp:66`, `chapter4.md:82/88/405`,
  and `voice_bible.md:51` — but set by nothing, so the entire GDD-named
  cold-senior cross-chapter arc was permanently unreachable. **Chose to
  re-author (b), NOT append (e)**, because `DialogLoader.cpp:83`
  hard-caps substate letters at `'a'..'d'`; appending (e) would be
  silently dropped by the loader (the brief was internally inconsistent
  with §6). Only `ending_a.txt` interacts with suit_senior (and picks
  `choose 2` = (d) HelpedSenior) — choice indices stay byte-fixed;
  endings A/B/C state.jsonl 2× byte-identical pre↔post. *Why:* a
  GDD-defined ripple option missing from the playable surface. *Impact:*
  the cold-senior arc (Ch2 −3 ripple, Ch3 學長迴避, Ch4 學長不出場) is
  finally reachable from in-game play. Karma economy unchanged (the
  −5 mirrors the existing (c) −5 受騙取陷阱傘 tier; well within the
  −15/−30 big-event ceiling per GDD §伍). **Note: the original (b)
  「玩家拒絕拿傘」 peaceful-decline beat is no longer reachable —
  user-acknowledged tradeoff vs the §6 4-substate cap.** *Revert:*
  `git revert 496a771`.
- **F1 (`b33db2b`) — remove inert Flag_KnowsUglyUmbrella per B3
  precedent.** `chapter1.md` (c) (玩家購買醜綠傘後) annotation
  `// Flag_KnowsUglyUmbrella = true` removed; `src/Harness.cpp`
  `KnownFlags()` state.jsonl whitelist cleaned; `src/EndingGate.cpp`
  comment header rewritten; `docs/SCRIPT_HANDOFF.md` "Ending C 觸發點"
  repointed to make explicit that the only Ending-C trigger is the Ch4
  集英樓 Vendor setting `Flag_BoughtUglyUmbrella` (read by
  `EndingGate.cpp:66`). The flag was set by content, read by no
  `src/`/`include/` code — same dead-annotation shape that B3 removed
  for `Flag_SawVictim_Ch1`. *Impact:* none on gameplay; the Ch1 阿姨
  (c) buy is now explicitly a "pure narrative seed" (karma +0, no flag,
  no Ending-C contribution); the `dialog_lint` engine whitelist
  (dynamically scanned) no longer lists the dead flag. *Revert:*
  `git revert b33db2b`.

### Why
A read-only narrative-conformance audit found two genuine GDD vs
implementation defects: F2 (an entire ripple arc dead-content); F1
(vestigial inert flag inconsistent with the post-B3 conventions). User
picked the two surgical fixes from the audit's recommendations.

### Gameplay impact
F2 makes the GDD-named cold-senior ripple playable for the first time
(the player can now confront the senior and trigger the Ch2/Ch3/Ch4
cold reactions the chapter content has long described); the
peaceful-decline (b) beat is repurposed as the consequence-bearing
confrontation. F1 is documentation-only — no behaviour change, no
economy change, no balance change. Endings A/B/C remain byte-identical
to the pre-fix baseline (7490/4834/4142 state lines; same sha256s).

### Cycle-8 gate (lead independently re-run)
`b33db2b == origin`, tree clean · bare `cmake -B build` configures
(CMake-4 shim) · **0 project-code warnings** (C++20
`-Wall -Wextra -Wpedantic`) · suite **289/289 / 4254 assertions / 0
fail / 0 skip** (+2 vs Cycle-7 287, both audit-F2 revert-verified) ·
`dialog_lint.py` exit 0 / 0 warn (`--strict` also clean) ·
`dialog_lint.py --list-flags` no longer lists `Flag_KnowsUglyUmbrella`
· endings A/B/C reach 結局 A/B/C at exactly **7490/4834/4142** state
lines = baseline, 2× byte-identical (sha256 `572503d4…` / `f498d4a4…`
/ `029cdad3…`), 0 rain-teleports, winnable; single-writer throughout
(no parallel agents, no worktrees — per the Cycle-6 environment
incident).

---

## 2026-05-19 — Cycle 7: requirements #4–#10 (single-writer, sequential specialists)

User requirements #4–#10 delivered by `game-team-lead` driving sequential
non-isolated specialists (the §Cycle-6 environment incident barred
parallel *worktree* agents). **Lead-verified re-gate GREEN, independently
re-run:** `a5148f6 == origin` (in sync), working tree clean; bare
`cmake -B build` configures (CMake-4 shim); build **0 project warnings**
(C++20 `-Wall -Wextra -Wpedantic`); suite **287/287, 4235 assertions,
0 fail / 0 skip** (+7 vs Cycle-6 280, every new test revert-verified);
`dialog_lint` exit 0; endings A/B/C reach `結局 A/B/C` at
**7490/4834/4142** state lines = known-good baseline, **0 rain-teleports**,
game winnable; ending C 2× byte-identical, integrated ending A 2× byte-
identical (`sha256 3eb4ab40…`). Lead also launched an independent
`ending_a` re-confirmation post-merge.

### What
- **#4 (`1df4a95`) vendor 「不買」 decline.** `OpenVendorMenu` appends a
  trailing `先不買，謝謝` choice (always opens a menu, even on empty
  stock); the decline index closes the dialog with **zero** economy
  mutation (no money/flag/inventory/event). Appended LAST ⇒ every stock
  index and the deterministic ending scripts (choose 0) are byte-
  unaffected. *Why:* a purchase must never be forced (user #4). *Revert:*
  `git revert 1df4a95`. Guard `tests/test_vendor_decline.cpp`
  (revert-verified, real GC loop); `test_i35` reconciled (Choices 1→2).
- **#10 (`1df4a95`) building-name CJK 缺字.** 8 ideographs
  (井/仁/勇/塘/夫/志/泳/雩) used by 7 building names occur in no
  `docs/content` file, so `View`'s `Inside: <name>` HUD rendered raylib's
  no-glyph `?`. Fix: bake the full 56-glyph `Buildings.h` set into
  `Font.h UiLiteralChars()` (same atlas path as the V1 ▼ fix). *Revert:*
  in `1df4a95`. Guard `tests/test_font_ui_glyphs.cpp` drives the real
  `Buildings.h` table (revert-verified; covers future renames).
- **#9 (`8bc9c9f`) distinct umbrellas + 遊戲說明.** Data-driven
  `UmbrellaStyle` ⇒ 4 distinct rect silhouettes + bold separated tints
  (True=cyan domed / Fragile=grey broken / ProfTrap=amber spiked /
  Cursed=violet drooping) — readable apart at map scale. Shared
  `GameHelp.h` ⇒ a title-screen 遊戲說明 item + a pause-menu 說明 item
  (menu now 繼續/說明/重新開始/離開), one source so they never drift;
  glyphs baked into the atlas. MVC-pure (render-only). *Revert:* in
  `8bc9c9f`. Guards `test_umbrella_render.cpp` (rewritten) +
  `test_menu_help.cpp` (real GC loop), revert-verified.
- **#6 (`8bc9c9f`) every market stall a different person.** Old code
  passed one `shop_auntie.png` to all ten (clones on a clean clone). New
  pure `VendorSprite.h` selector keys off each stall's unique 攤主 +
  a distinct curated fallback per spawn index (ships-only sprites, no new
  art — §5). *Revert:* in `8bc9c9f`. Guard
  `test_vendor_centred_cluster.cpp` (pairwise-distinct).
- **#7 (`8bc9c9f`) market at 羅馬廣場 centre.** Ten stalls gathered into
  a verified-walkable bullseye at plaza centre **(1088,960)**: centre +
  r42 + r78 rings, every pair > 35 px (no overlap), max r78 ≪ the
  verified r100 walkable box. *Revert:* in `8bc9c9f`. Guards
  `test_vendor_centred_cluster.cpp` + `test_spawn_reachability`;
  `test_vendor_loader.cpp` reconciled.
- **#5 (`a5148f6`) rain pressure in EVERY chapter.** Root cause: the Ch1
  TrueUmbrella granted *permanent* rain immunity (`ApplyRain` self-noops
  with an umbrella **and** the GC treated "has umbrella" as full shelter
  ⇒ `DrainRain`), so Ch3 was literally **0.0** every frame. Fix: new
  `Player::ApplyRainSheltered` (+1.5 u/s ≈ 30 % of the exposed +5,
  clamp [0,100], lethal-armed) + a 3-way GC tick — inside a building →
  `DrainRain` (−10); outdoors + umbrella → `ApplyRainSheltered`;
  outdoors no umbrella → `ApplyRain` (+5, **byte-unchanged**). An
  umbrella now buys *time*, not immunity (chapter2.md's explicit "still
  accrues, reduced rate"). Per-chapter rain max now Ch1≈38 / Ch2≈52–81 /
  Ch3≈49–62 / Ch4≈57–66 — genuine pressure every chapter; competent play
  (long in-building dialog stretches drain hard) never trips 100, all
  scripts 0 teleports, Ch1 byte-identical. *Revert:* `git revert
  a5148f6`; the 1.5 u/s rate is tunable in `Player::ApplyRainSheltered`.
  Guard `tests/test_rain_survival.cpp` (3-way contract + sheltered unit,
  revert-verified).
- **#8 — verified complete, no code change.** A full-spine trace proves
  every chapter has spawned, reachable, firing quest-drivers: Ch2 學霸
  (clear) + 圖書館管理員 (info); Ch3 香腸→大聲公→學姊 chain; Ch4 助教
  finale. The perceived gap was the prior-cycle I3/I5/I6 reachability
  bugs (already fixed). *Decision:* confirming a working trace-verified
  system beats fabricating churn into a deterministic green spine (§5/§7).

### Autonomous design decisions (GDD-consistent — flagged for redirect)
- **#5 rate** umbrella-outdoors **1.5 u/s** (~30 % of exposed) — felt
  every chapter (peaks 50–80) yet never lethal under competent play;
  tunable in `Player::ApplyRainSheltered`.
- **#7 centre** world **(1088,960)** (densest stone blob; verified
  all-walkable r≈100 box); compact bullseye so the market reads "dead
  centre" without overlap or blocking road junctions.
- **#9 palette** silhouette **and** colour both differ so the four
  umbrellas read apart even at map scale / on subtle displays.
- **#8 no-change** the requirement is already met (trace-verified);
  forcing churn was judged a regression risk with no upside.



Two agents ran in parallel (logic-bug hunt in the main tree; UI in an
isolated worktree). Disjoint file sets ⇒ conflict-free merge `b26aa9a`.
**Lead integrated re-gate GREEN (main tree):** bare `cmake -B build`
configures (CMake-4 shim `6bf5a5e`); build **0 project warnings**
(C++20); suite **280/280, 3601 assertions, 0 fail/0 skip** (incl. A's
`test_ending_gate`/`test_ripple_seed_flags` + B's `test_restart_safety`);
`dialog_lint` exit 0; playtest gate (8-script no-crash + determinism)
re-run by the lead post-merge.

### What
- **L1 (`d3b25ab`) — Ch4 助教 finale gate made TOTAL.**
  `src/EndingGate.cpp` +35/-5: once `Flag_TaFinaleChoiceMade` is set the
  gate is total — cold finale → Ending B (GDD §陸 B 字卡), flag-only →
  Ending C (破財消災 default); pre-finale Ch4 free-roam byte-unchanged;
  A→B→C precedence preserved. Fixes a real **unwinnable soft-lock**
  (an honest player who declines compassion fell through A/B/C and was
  stuck forever). Regression `tests/test_ending_gate.cpp`.
- **L2 (`4c36337`) — §5 `isActive_` idempotency guard restored.**
  `src/TrueUmbrella.cpp:7` & `src/FragileUmbrella.cpp:7`
  `if(!isActive_)return;` (parity with Cursed/ProfessorTrap). Closes a
  latent double-`UmbrellaClaimed` → double semester-transition hazard.
  Regression `tests/test_ripple_seed_flags.cpp`.
- **UI cycle (`6d9ca97`, agent B).** Title screen; 5 non-binary personas
  (runtime colour-tint over already-shipped sheets — no new assets, §5);
  always-on money HUD (`金幣: N 元`); in-game top-right pause menu
  (繼續/重新開始/離開, sim frozen while open; Restart rebuilds
  {World,View,GameController}). Harness still auto-bypasses title +
  select (`UMBRELLA_SPRITE`); `.claude/scripts/*` unchanged & still reach
  gameplay. Restart-safety regression `tests/test_restart_safety.cpp`
  (karma/money/flags reset; no EventBus dangle — BUGLEDGER B2/H1).

### Why
The user disputed the Cycle-5 "converged" verdict — correctly: live play
of all 8 scripts surfaced L1 (severe, unwinnable) + L2 (§5 red-line).
The UI shell delivers the requested start flow / character identity /
money visibility / in-game menu.

### Revert
`git revert b26aa9a d3b25ab 4c36337` (or reset to `6bf5a5e`); delete the
new `tests/test_ending_gate.cpp` / new cases in `test_ripple_seed_flags
.cpp` / `tests/test_restart_safety.cpp`; UI files revert with `6d9ca97`.

---

## 2026-05-19 — Cycle 4: lethal rain ACTIVATED via a drain rule (I8 fully resolved)

Gated GREEN: build **0 project warnings** (C++20); suite **274/274,
3564 assertions, 0 fail/0 skip**; `dialog_lint` exit 0; harness A/B/C
under lethal+drain — all reach `結局 A/B/C`, **2× byte-identical**, no
soft-lock, **0 teleports**, line counts = baseline (7490/4834/4142).

### What
- `Player::DrainRain(float dt) noexcept` — rain recovery −10 u/s,
  clamped [0,100], no teleport (mirror of `ApplyRain`'s +5 u/s accrual).
- `GameController::Update()` rain tick rewritten: skipped only in the
  market interlude & endings; otherwise **SHELTERED** (has umbrella OR
  `CurrentBuildingName()` non-empty) ⇒ `DrainRain(dt)`; **EXPOSED**
  (outdoors, umbrella-less) ⇒ `ApplyRain(dt, /*lethal=*/true)` — the
  ≥100→正門 `RespawnAtGate` teleport is now LIVE.
- `tests/test_rain_exposure_conservative.cpp` → renamed
  `tests/test_rain_survival.cpp`, rewritten as the Cycle-4 regression
  (DrainRain unit; GC accrue→lethal-gate-fires; umbrella⇒GC-drains;
  market no-tick). `test_player_core.cpp` ApplyRain case untouched.

### Why
Cycle 3 deferred lethal rain (would derail the deterministic scripts).
The user (this cycle) chose to activate it via a drain rule. Perceive
proved a *bounded* "flip lethal + re-route" was **infeasible**: the
meter was monotonic (no drain), and Ending A structurally needs a
~28 s umbrella-less Ch1 quest chain vs a 20 s budget ⇒ a 10 300-
teleport Ch1 doom-loop (B/C unaffected). The missing piece was always
a recovery path. With −10 u/s drain while sheltered, the **existing
scripts already shelter enough** (dialogs inside buildings + umbrella
claims) that net rain peaks 38/35/68 and never hits 100 — so the GDD
survival pillar is finally real *and* lethal with **zero script
re-authoring**.

### Gameplay impact
Rain is now a genuine manage-your-exposure loop: stay out umbrella-less
too long → swept back to 正門 (half-day lost). Competent play (grab the
umbrella, duck through buildings) keeps you dry — exactly the GDD
fantasy. Endings/karma/routes unchanged (the scripts are competent
runs; determinism preserved, re-verified 2×).

### Revert
GC tick back to the Cycle-3 conservative block (`ApplyRain(dt,
/*lethal=*/false)`, outdoor-gated, no drain branch); delete
`Player::DrainRain`; restore the test to the conservative version
(`git mv` back + revert content). Tunables: accrual 5 u/s
(`Player.cpp` ApplyRain), drain 10 u/s (`Player.cpp` DrainRain).

---

## 2026-05-19 — Cycle 3 R2: experience pass (rain wired · F2 · F3a · V1/V2/V3)

Gated GREEN on the main thread: from-scratch build **0 project warnings**
(C++20, `-Wall -Wextra -Wpedantic`); suite **273/273, 3436 assertions,
0 failed/0 skipped** (incl. 4 new revert-verified doctests + 2 F2 test
reconciliations); `dialog_lint` exit 0 / 0 WARN; harness re-verified —
`ending_{a,b,c}` still reach `結局 A/B/C`, **2× byte-identical**, no
soft-lock, frame counts unchanged vs the pre-rain baseline (7490/4834/
4142) while `rain` now peaks 100/71.5/89.3 (was 0 every frame).

### I8 — Rain-survival meter wired (CONSERVATIVE; see BUGLEDGER I8)
- **What:** `Player::ApplyRain` gained `bool lethal = true`;
  `GameController::Update()` now ticks `ApplyRain(dt, /*lethal=*/false)`
  once per frame when the player is OUTDOORS (semester ∉
  {Interlude_Market, Ending_*} **and** `World::CurrentBuildingName()`
  empty). `lethal=false` accumulates+clamps [0,100] but suppresses the
  ≥100→正門 `RespawnAtGate`. Umbrella still self-nullifies (unchanged).
- **Why:** the GDD's core survival mechanic was dead code — `ApplyRain`/
  `RespawnAtGate` had **zero** production callers (triple-confirmed:
  balance audit + grep + a 3000-frame idle probe; `rain==0` for all
  16 466 trace frames).
- **Impact:** rain genuinely fills outdoors and is observable; umbrella/
  shelter still dry; the player feels exposure pressure via the new HUD
  readout + vignette (V2). **No movement-speed penalty, no teleport this
  cycle.** Endings/karma/routes byte-unchanged (rain is consequence-free
  this cycle ⇒ deterministic `goto` scripts intact — verified).
- **DEFERRED (explicit, future dedicated cycle):** the lethal
  ≥100→正門 teleport + half-day, AND any movement-speed/slow penalty —
  both would perturb the deterministic ending scripts; out of scope.
- **Revert:** drop the `ApplyRain` call in `GameController::Update()`;
  optionally restore `ApplyRain` to one-arg.

### F2 — CursedUmbrella karma penalty 50 → 30
- **What:** `CursedUmbrella::karmaPenalty_` 50→30 (+ stale event text
  `"Karma -50"`→`"Karma -30"`). Two pre-existing tests that hard-coded
  the old −50 reconciled to −30 (`tests/test_ripple_seed_flags.cpp:39,
  43`, `tests/test_factory.cpp:49`) — post-design-change constant
  update, test intent preserved (the I6 precedent).
- **Why:** GDD/SCRIPT_HANDOFF lock big-event karma at −15/−30; −50 was
  67 % over the ceiling.
- **Impact:** stealing the cursed umbrella costs −30 (claim 50→20, was
  0). **Ending B reachability UNCHANGED** — `EndingGate` routes on
  `Flag_TookCursedUmbrella` (always set by `beClaimed`).
- **Revert:** `karmaPenalty_(30)`→`(50)`, event text, the 3 test
  constants.

### F3a — Ending B `karma<0` clause documented (doc-only)
- **What:** one clarifying sentence in `遊戲企劃與敘事架構.md` Ending-B
  trigger: normal play floors karma well above 0 (clamped, starts 50),
  so Ending B is in practice `Flag_TookCursedUmbrella`-driven; the
  `karma<0` clause is retained in code as a defensive lower bound.
  `EndingGate.cpp` byte-unchanged. *Revert:* delete the sentence.

### V1/V2/V3 + rain feedback — gfx/UX (View/Font/EndingView only)
- **V1:** added U+25BC ▼ (+ V3 caption ideographs / CJK quotes) to
  `gfx/Font.h::UiLiteralChars()` — B4's "▼ more" pagination cue was
  absent from the atlas → rendered as `?`. *Revert:* drop the literals
  (`tests/test_font_ui_glyphs.cpp` then fails, by design).
- **V2:** top-left HUD now panel-backed (`Color{20,22,30,185}` +
  bright text, the objective-panel idiom) + a `rain: NN%` readout
  (White→Gold≥60→Red≥85) — was `DarkGray`/`Blue` on bright map,
  illegible. *Revert:* restore the 4 plain TextBuilder lines.
- **Rain vignette:** screen-edge darkening tiers (rm≥60 α45, ≥85 α90)
  in `View.cpp`, pure render from `GetRainMeter()` (MVC-safe, zero
  sim/determinism effect) — the non-lethal "pressure" feedback this
  cycle. *Revert:* delete the vignette block.
- **V3:** authored real Ending A (`「雨過天晴，傘還在你手上。」`) & B
  (`「你成為了你曾經最討厭的那種人」`, grey-tinted per GDD §陸) captions
  replacing the `（待 Phase 2 接入）` placeholders; title/caption
  centred via `nccu::dialog::CellWidth` instead of hardcoded x offsets.
  *Revert:* restore placeholder caption + hardcoded offsets.
- **Guard:** `tests/test_font_ui_glyphs.cpp` (revert-verified — atlas
  has 0x25BC + the V3 caption glyphs); `test_ending_card_render.cpp`
  still green (still 1 DrawRect + 2 DrawText).

---

## 2026-05-18 — Cycle 2 (in progress): B5 content/lint drift resolved

### B5 — 21 dialog_lint WARNs driven to 0 (see BUGLEDGER B5)
- **What — content:** 4 reactive beats that were authored as
  unsupported inline conditionals (`*（若 Flag_X = true）* "…"`, which
  the loader silently dropped) re-authored in `docs/content/chapter2.md`
  (+ `chapter1.md`/`chapter4.md`) as genuine flag-gated **separate
  substates**: 學霸 圖書館初遇 `(b)` `Flag_TookCursedUmbrella` (default
  `(a)` = un-cursed常態; broadcast-notes sub folded into a
  `QuestFlagPickup`/`ShowMessage` note, `// karma +3` lands once);
  福利社阿姨 `(b)` `Flag_BoughtUglyUmbrella`; 苦主 `(a)` split
  `Flag_PromisedVictim`. `src/DialogOpener.cpp` `ResolveOpenerSubState`
  routes Ch2 to the reactive substate iff its flag is set (same routing
  pattern as B3's Ch4 fix).
- **What — lint:** `dialog_lint.py` flag-prose heuristic refined — a
  prose cross-reference of an engine-**whitelisted** flag no longer
  WARNs; only a genuinely unknown/unwired flag in prose WARNs (real
  dead-flag detection, the B3 capability, is preserved).
- **Why:** the flags were wired but their reactive payoff was mute (the
  loader has no inline-conditional production), and the lint over-flagged
  every documentary mention of a real flag — 17 false + 4 genuine WARNs.
- **Impact:** carrying the cursed umbrella / having bought the ugly
  umbrella / having promised the victim in Ch1 now actually changes the
  Ch2 encounter line as the docs always claimed. No economy/ending-gate
  change. Lines that already fit are byte-unchanged.
- **Verification:** `dialog_lint.py docs/content/*.md` exit 0, **0 WARN**
  (was 21), main-thread-verified 06:02Z; new revert-verified doctest
  `tests/test_ch2_reactive_substates.cpp`. Integrated ctest/zero-warn
  re-gate deferred to the next main-thread gate (avoid racing the
  in-flight harness-verb `build_hv`); Cycle 2 stays open until it lands.
- **Revert:** `git checkout` `docs/content/chapter2.md`+`chapter1.md`
  +`chapter4.md` and the Ch2 routing hunk in `src/DialogOpener.cpp`;
  delete `tests/test_ch2_reactive_substates.cpp`; restore the prior
  `dialog_lint.py` flag-prose heuristic.

## 2026-05-18 — Cycle 1: B3 dead-flag fix · C++20 · H1 EventBus RAII

### B3 — two dead quest flags resolved (see BUGLEDGER B3)
- `Flag_BoughtCoffeeForAuntie_Ch1` — **wired**. New Ch1 福利社阿姨 (d)
  請咖啡 `DialogChoice` (`setsFlag`, `// karma +5`); `ResolveOpener
  SubState` routes Ch4 `shop_auntie` → subState 0 (直接情報) if set
  else 3 (間接情報); `chapter4.md` 阿姨 split (a) direct / (d) indirect;
  `TryApplyCh4Ripple` grants +3 once (`Flag_Ch4Rippled_Auntie` guard).
  *Why:* a named GDD §2.2 generosity flag was set by content but read
  by nothing — the intended Ch4 direct-info branch never fired.
  *Impact:* buying 阿姨 a coffee in Ch1 now yields +5 then a +3 Ch4
  ripple and direct umbrella intel. *Revert:* `git checkout` the 5
  prod/content files + 3 test cases (test_ch4_ripple/​dialog_opener).
- `Flag_SawVictim_Ch1` — **removed** (dead annotation; the 苦主 ripple
  is already driven by live `Flag_PromisedVictim`). *Impact:* none —
  it gated nothing. *Revert:* restore the chapter1.md note line.
- *Guard:* dialog_lint exit 0 (was 3 ERRORs); 3 revert-verified
  doctests. Files: `src/DialogOpener.cpp`, `src/Chapter4Quest.cpp`,
  `include/Chapter4Quest.h`, `docs/content/chapter1.md`+`chapter4.md`,
  `tests/test_ch4_ripple.cpp`+`test_dialog_opener.cpp`.

### Build — C++ standard raised 17 → 20
- `CMakeLists.txt`: `CMAKE_CXX_STANDARD 20`. *Why:* user lifted the
  C++17 ceiling; zero-warnings is the real red line, not the language
  version. *Impact:* none on gameplay; full gate re-verified green
  under C++20 (232/232 tests, 0 project warnings, dialog_lint exit 0).
  *Revert:* set it back to `17`.

### H1 — EventBus RAII scoped unsubscribe (see BUGLEDGER H1)
- **What:** added `EventBus::Subscription` (movable/non-copyable RAII
  handle) + `[[nodiscard]] EventBus::ScopedSubscribe(EventType,
  Handler)`. The token's destructor removes exactly its handler
  (stable id-keyed storage). `Subscribe`/`Clear` unchanged, fully
  backward-compatible.
- **Why:** subscriptions captured caller-owned state with no scoped
  teardown — correctness hinged on manual `Clear()` before captures
  died (the B1/B2 use-after-free). The handle ties a subscription's
  lifetime to a scope/owner so subscribers can't dangle.
- **Impact:** no gameplay/behavior change; engine-safety hardening
  only. Existing call sites + the test-isolation listener still work.
- **Revert:** restore prior `include/EventBus.h`/`src/EventBus.cpp`,
  delete `tests/test_eventbus_scoped.cpp`.

### B4 — dialog box CJK-aware wrapping + pagination (see BUGLEDGER B4)
- **What:** long dialogue lines now wrap to the 28-cell box and
  paginate (3 rows/page) with a "▼" more-affordance; the existing
  advance key (Space/Enter/E) turns pages. New pure `DialogLayout`
  (CellWidth/WrapToCells/Paginate); `DialogState` gains a page cursor;
  `DialogView` renders per-page. `dialog_lint.py`'s raw-length WARN is
  now a renderer-contract assertion (ports `WrapToCells`).
- **Why:** 251 authored lines (30–54 cells) overflowed/clipped the box.
  Content is correct; the renderer had to handle long lines — CLAUDE.md
  §6 box width is now an engine guarantee, not just a lint.
- **Impact:** no content rewrites. Long conversations read across pages
  instead of off-screen; one extra keypress per extra page. Choice
  labels also wrap. Bit-for-bit unchanged for lines that already fit.
- **Revert:** delete `include/DialogLayout.h`, `src/DialogLayout.cpp`,
  `tests/test_dialog_layout.cpp`; revert `DialogState.{h,cpp}` +
  `DialogView.cpp` to single-`DrawText`; restore the raw-length WARN.

## 2026-05-17 — Bootstrap: harness + green baseline

### Engine fixes (git-tracked; see BUGLEDGER B1/B2)
- `src/EventBus.cpp`: add `#include <mutex>`. Was non-compiling under
  modern libstdc++. *Impact:* none on gameplay (build-only). *Revert:*
  drop the include.
- `tests/test_eventbus_isolation.cpp` (new): doctest listener clearing
  the global EventBus at every test/subcase boundary. Fixes the
  cross-test dangling-handler SIGSEGV/double-free. *Impact:* test-only;
  production untouched. *Revert:* delete the file.

### Autoplay harness (git-tracked — new capability, zero play change)
- `include/gfx/Input.h`: introduce `InputSource` + `LiveInput`; `Input`
  facade now delegates to a swappable source (default = live raylib).
  All call sites unchanged. *Why:* single seam to inject scripted input.
- `include/gfx/Time.h`: optional fixed timestep (`SetFixedStep`); unset
  ⇒ real frame time. *Why:* deterministic replay.
- `include/ScriptInput.h` + `src/ScriptInput.cpp` (new): deterministic
  timeline `InputSource` with raylib-faithful edge semantics.
- `include/Harness.h` + `src/Harness.cpp` (new): env-driven autoplay —
  scripted input, per-frame JSON state dump, screenshots after
  `EndDrawing`, maxframes watchdog, EventBus event capture. Inert unless
  `UMBRELLA_SCRIPT` set.
- `src/main.cpp`: +`MaybeAttach`/`WireEvents`/`BeginFrame`/`EndFrame`
  wiring; harness-active runs skip character-select with a fixed sprite.
  Composition-root spirit preserved.
- `tests/test_scriptinput.cpp` (new): locks edge/hold/press/quit/parse
  semantics (4 cases, +27 assertions).

*Gameplay impact:* **none** — verified normal play (no `UMBRELLA_SCRIPT`)
is bit-for-bit unchanged: window opens, runs >5 s, no crash, no stray
artifacts. *Revert:* remove the new files and the four `harness.*` lines
+ char-select branch in `main.cpp`; revert `Input.h`/`Time.h`.

### Validation
- Build: zero project-code `-Wall -Wextra -Wpedantic` warnings.
- Tests: 229/229 cases, 1098/1098 assertions, 0 skipped.
- Headless: `xvfb-run` + llvmpipe (GL 4.5 core). Scripted run → valid
  800×450 PNGs + 361-line `state.jsonl`; two runs byte-identical
  (deterministic).

### Team apparatus (local only, `.claude/` + `CLAUDE.md`, gitignored)
- `CLAUDE.md`, `.claude/settings.json` (allowlist + SessionStart hook),
  `.claude/tools/playtest.sh`, `.claude/scripts/smoke_walk.txt`,
  `.claude/BUGLEDGER.md`, this file.

_No GDD/design changes yet — baseline economy/narrative/endings intact._

---

## 2026-05-18 — Cycle 2 perception: A/B/C ending scripts committed; endings proven UNREACHABLE (3 engine defects found)

- **Added** committed progression scripts
  `.claude/scripts/ending_{a,b,c}.txt` (robust plan-verbs:
  `interact/goto/choose/advance/wait/quit`, fully commented), authored
  to the authoritative `EndingGate` design (A: `karma>80 &&
  Flag_HasTrueUmbrella && Flag_ConsoledTA`; B: `Flag_TookCursedUmbrella
  || karma<0`; C: `Flag_BoughtUglyUmbrella`).
- **Why:** convergence bar rows 5 (3 endings reachable, no
  crash/soft-lock) & 6 (deterministic replay) — first end-to-end
  exercise of the A/B/C spine through the harness.
- **Result:** each script runs 2× rc=0, 0 crash, **replay
  byte-identical** (sha256 A `cacdefba…23ce`, B `ffa64daf…4359`, C
  `baaffe62…4392`; C re-verified on the main thread) ⇒ **row 6 infra
  GREEN**. But **no ending is reached** — all three soft-lock in Ch1
  (`semester` never leaves `第一章 加退選`, no `結局`, no flag set) ⇒
  **row 5 RED**. Root cause = 3 engine defects, all verified from
  source, filed as BUGLEDGER **I3/I4/I5** (NOT fixed in this entry —
  perception pass): I3 interact-NPC geometrically impossible
  (movement-blocker box == E-interaction box; strict
  `Rect::Intersects`; the "BUGLEDGER I3" `test_scriptinput_plan.cpp`
  already cites but was never written); I4 `ScriptInput.cpp:196`
  ResolvePlan force-releases classic WASD every frame for plan-less
  scripts (regression from the Cycle-2 harness-verb integration); I5
  `Vendor::TryBuy` has no runtime caller (empty `npcId_` ⇒ routed to
  `NPC::Interact`).
- **Gameplay impact:** none from this entry (no `src/include/tests/
  docs` or `build/` change); it documents that the game is **currently
  unwinnable** (I3 also affects normal human play; I5 disables all
  vendors) — fixes tracked under I3/I4/I5.
- **Revert:** delete the three `.claude/scripts/ending_*.txt`.

---

## 2026-05-18 — I3/I4/I5 FIXED (game human-winnable again); I6 found

- **I3 (correctness, `src/GameController.cpp`):** E-interaction now has
  an explicit 8px reach margin (`kInteractReach`; probe inflated to
  `Rect{px-8,py-8,40,40}`) so a player flush-blocked against a static
  NPC can still talk. The movement collider (`frameColliders_`,
  `ResolveMove`, `Rect::Intersects`) is byte-for-byte UNCHANGED — no
  walk-through. Applies to human + harness alike (the intended
  correctness fix). Guard: `tests/test_i35_interact_vendor.cpp` (open
  + no-pass-through, revert-verified).
- **I4 (regression, `src/ScriptInput.cpp`):** `ResolvePlan`
  early-returns when `plan_.empty()` BEFORE any `releaseMoveKeys()`, so
  a classic-only timeline's `down`/`press` key state is no longer
  force-released every frame. Plan-verb scripts byte-identical. Guard:
  `tests/test_scriptinput_classic_move.cpp` (revert-verified).
- **I5 (correctness, `src/GameController.cpp` + `include/GameController.h`
  + `include/GameObject.h` + `include/Vendor.h`):** virtual
  `IsVendor()`; GameController opens a buy-choice dialog and drives
  `Vendor::TryBuy` on confirm. ALL economy side-effects stay inside
  `Vendor::TryBuy` (DeductMoney/AddConsumable/EventBus/soft-cap/
  setsFlag) — `test_vendor` contract intact. Ch2 EnergyDrink now
  obtainable in-engine. Guard: `tests/test_i35_interact_vendor.cpp`
  (Ch4 buy + Ch2 clear, revert-verified).
- **Gameplay impact:** the game is **winnable by a human again** —
  NPCs are interactable and vendors functional; spine
  Ch1→Ch2→Ch3→Ch4 + all 3 endings reachable in-engine (doctest-proven
  via the real GameController loop). No rebalancing — pure
  wiring/geometry correctness. Integrated-gated 08:31Z: 0 project-warn
  C++20, 261/261 tests 0 skipped, dialog_lint 0/0.
- **Known follow-up (BUGLEDGER I6, OPEN):** the autoplay `interact`
  plan-verb still cannot drive these interactions (its arrival target
  is behind the movement blocker so it never presses E), so the
  committed A/B/C scripts still soft-lock — scripted ending proof is
  pending the I6 fix; human playability is not affected.
- **Revert:** `git checkout` the listed `src/`+`include/` files; delete
  `tests/test_i35_interact_vendor.cpp` and
  `tests/test_scriptinput_classic_move.cpp`.

---

## 2026-05-18 — I6 FIXED: harness `interact` verb + `interact <x> <y>`; Cycle 2 converged (7/7)

- **I6 fix (correctness, `src/ScriptInput.cpp` only):** `Verb::Interact`
  now drives straight at the NPC ORIGIN and presses E gated on the SAME
  inflated-AABB overlap test the landed I3 GameController fix uses
  (`kInteractReach = 8.0f`; `pHit{pos.x-8,pos.y-8,40,40}`;
  `npc->CheckCollision(pHit)` — mirrors `GameController.cpp:293-300`),
  NOT on `AxisKeyToward` arrival (the old `npos.x-8` target sits behind
  the strict-collision flush-stop and is unreachable — an arrival gate
  reproduced the I6 soft-lock). `kAdjacentDx` removed (now unused; would
  otherwise warn under `-Wall -Wextra -Wpedantic`). Pure deterministic
  function of the two live positions.
- **Harness-grammar extension (tooling, design-relevant):** added
  `interact <label> <x> <y>` to the script grammar. **Why:** umbrellas,
  `QuestFlagPickup`s and Vendors all have an empty `NpcId()`, so the
  plan had NO way to E-actuate any non-NPC world object — every ending
  must claim the TrueUmbrella (and Ch1/Ch3 items), so the whole A/B/C
  spine was un-actuatable by the autoplay plan. The coordinate form
  drives to (x,y) and taps E via the same I3-mirror reach geometry,
  handing off when a Vendor menu opens (no double-buy) or the player
  reaches the origin (silent pickup). **Backward-compatible:**
  `interact <npcId>` with no coords is byte-unchanged; an id that
  resolves to a live NPC still takes the NPC arm (coords ignored). Doc
  comment in `Load()` records the grammar.
- **Gameplay impact:** the autoplay harness can now drive the full
  spine; all three endings are reachable AND deterministic by the
  harness — `結局 A` (8906f, karma 100) / `結局 B` (5734f, karma 28) /
  `結局 C` (5107f, karma 63), each 2× byte-identical
  (`sha 3368e007…`/`4469969a…`/`223cc0ca…`). Combined with I3, the
  game is winnable to all three endings by both a human and the
  harness. No karma/economy rebalance in this entry — pure
  wiring/geometry + tooling correctness. The committed
  `.claude/scripts/ending_{a,b,c}.txt` were rewritten to the runnable
  spine (the prior ones encoded the intended design but soft-locked).
  **Harness-only:** the change is confined to `Verb::Interact`/`Load()`,
  which only execute for plan-bearing scripts (classic scripts
  early-return at `plan_.empty()`; the harness is inert unless
  `UMBRELLA_SCRIPT` is set) ⇒ normal play is bit-for-bit unchanged
  (CLAUDE.md §5).
- **Test reconciliation:** `tests/test_scriptinput_plan.cpp`'s
  `interact victim` case asserted the pre-I3 flush-exactly-24
  endpoint; it was green at the 08:31Z gate only because the I6 bug
  soft-locked the verb at the flush stop. Updated to the post-I3/I6
  reach band (`|last.x-vx| <= 24+8`) with rationale + a cross-reference
  to `tests/test_i6_interact_reach.cpp` (the new doctest that owns the
  dialog-opening guarantee). This is a post-design-change test update,
  not a code defect.
- **Revert:** `git checkout` / restore `src/ScriptInput.cpp` and
  `tests/test_scriptinput_plan.cpp`; delete
  `tests/test_i6_interact_reach.cpp`; restore the prior
  `.claude/scripts/ending_{a,b,c}.txt` (they will soft-lock again).

## Cycle 5 (2026-05-19) — NO functional change

- Assessed the deferred F5-pacing / F7-money cycle + a grounded UI
  verdict. **No `src/include/tests/docs/content` change made** — F7
  was already resolved by the Cycle-4 live rain economy and F5
  asymmetry is by-design (earned ending A longest; B/C deliberate
  shortcuts). UI verified DONE from pixels (fully-assetted env).
  Full rationale, trace evidence and gate re-verification:
  `.claude/ACCEPTANCE.md` → "Cycle 5". Nothing functional to log
  here; recorded for ledger continuity.
- **Revert:** N/A — the tree was never modified (clean at `923059c`).

## UI Cycle (2026-05-19) — title screen, 5 personas, money HUD, in-game menu

- **WHAT:** Four UI/UX features. (1) 金幣 HUD: a live gold "金幣: N 元"
  line added to the in-game status panel (`src/View.cpp`), read-only
  from `Player::GetMoney()` (MVC pure). (2) 首頁 title screen
  (`include/TitleScreen.h` + `src/TitleScreen.cpp`): "尋傘記 / 政大山下
  篇" + keyboard menu 開始遊戲 / 離開, shown before gameplay. (3) 5
  non-binary personas replacing the binary gender pick
  (`include/CharacterSelect.h` + `src/CharacterSelect.cpp`): 夜貓子 /
  social咖 / 邊緣人 / 卷王 / 佛系生, each an EXISTING Pipoya sheet +
  a runtime `DrawTexturePro` colour tint (`Player::SetTint`, applied in
  `Player::Render`) so all 5 read distinct with NO new asset binary
  (CLAUDE.md §5). (4) In-game pause menu (Esc/M, top-right "ESC 選單"
  affordance): 繼續 / 重新開始 / 離開, pure-data on `World`
  (`MenuOpen`/`MenuCursor`/`PendingAppAction`, the InventoryOpen idiom),
  driven by `GameController`, rendered by `View`; freezes the sim while
  open. `src/main.cpp` becomes an outer Title→Select→Playing screen-flow
  loop; 重新開始 tears down the per-run {World,View,GameController}
  scope (RAII → `~GameController` Clears the EventBus before the next
  World is built — BUGLEDGER B2/H1 safe) and rebuilds fresh.
- **WHY:** Player must always see money; a proper home screen + a
  restartable run + an inclusive non-gendered cast are the requested
  UX. Tint-mapping satisfies the no-new-asset red line.
- **GAMEPLAY IMPACT:** No balance/economy/quest numbers changed. New
  pre-game screens (human only). Restart = a clean fresh game (karma 50
  / money 100 / no flags) — proven by `tests/test_restart_safety.cpp`.
  HARNESS UNCHANGED: with UMBRELLA_SCRIPT set both new screens are
  bypassed exactly as the old char-select was; smoke/ending state.jsonl
  is BYTE-IDENTICAL to the pre-feature baseline (verified).
- **REVERT:** `git checkout` `include/CharacterSelect.h`
  `include/Player.h` `include/World.h` `include/gfx/Font.h`
  `src/CharacterSelect.cpp` `src/GameController.cpp` `src/Player.cpp`
  `src/View.cpp` `src/main.cpp`; delete `include/TitleScreen.h`
  `src/TitleScreen.cpp` `tests/test_restart_safety.cpp`; re-run cmake
  (GLOB). Old binary-gender 2-phase select + direct
  Title-less main loop return; harness path was already identical so
  nothing there to revert.
