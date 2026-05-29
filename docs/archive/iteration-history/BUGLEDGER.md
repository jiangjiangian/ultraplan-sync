# BUGLEDGER — 《尋傘記》

Living defect register for the self-iterating team. Every entry: symptom →
root cause → fix → regression guard. Architectural hazards that are not
yet fixed live under "Open / hardening" so the architecture-reviewer can
pick them up in the loop.

Status legend: [FIXED] verified by gate · [OPEN] not yet addressed.

---

## Bootstrap bug-hunt (baseline did not build/pass on a fresh clone)

### B1 [FIXED] EventBus.cpp fails to compile under modern libstdc++
- **Symptom:** `cmake --build` fails — `error: 'unique_lock' is not a
  member of 'std'` in `src/EventBus.cpp` (Subscribe/Clear). Baseline did
  not build at all on this toolchain (GCC 13+ / libstdc++).
- **Root cause:** `EventBus.cpp` uses `std::unique_lock` (header
  `<mutex>`) but only `<shared_mutex>` was transitively included via
  `EventBus.h`. Older libstdc++ pulled `<mutex>` in through
  `<shared_mutex>`; newer libstdc++ does not.
- **Fix:** add `#include <mutex>` to `src/EventBus.cpp`.
- **Guard:** the build itself; suite cannot link without it.

### B2 [FIXED] SIGSEGV / "double free" — global EventBus dangling handlers
- **Symptom:** `OOP_Raylib_Lab_tests` crashes: `SIGSEGV` at
  `test_cashpickup.cpp:54`, then (once past that) `free(): double free
  detected` during `Player::ApplyRain`. Crash aborts the run → 202 of
  225 cases reported "skipped". Suite was never actually green.
- **Root cause:** `EventBus` is a process-global singleton with **no
  per-handler unsubscribe** — only a global `Clear()`. Many tests
  `Subscribe` a lambda capturing a stack `MessageCapture`/`[&]` local and
  rely on the *next* test's `Clear()` to remove it. A test that
  `Publish()`es before its own `Clear()` (the CashPickup factory cases;
  the `ApplyRain` respawn) invokes a handler over a destroyed capture →
  use-after-scope.
- **Fix:** one test-isolation listener,
  `tests/test_eventbus_isolation.cpp`, clears the bus at every test /
  subcase boundary, so a handler can never outlive the scope that
  registered it. Single systemic fix — no per-file patching, no engine
  API churn. Production is unaffected (only `GameController` subscribes
  there and its dtor `Clear()`s).
- **Guard:** full suite now runs hermetically — 229/229 cases,
  0 skipped. Previously crashing cases pass.

---

## Open / hardening (for the architecture-reviewer in the loop)

### H1 [FIXED] EventBus has no RAII / scoped unsubscribe
- **Root cause:** `Subscribe` stored a capturing `std::function` with no
  per-handler removal — only global `Clear()`. Correctness depended on
  every subscriber manually `Clear()`-ing before its captured refs/`this`
  died (the B1/B2 dangling-capture UAF footgun).
- **Fix:** added `EventBus::Subscription` — a non-copyable, movable RAII
  token; its dtor/`Reset()` removes exactly its handler by stable id
  (id-keyed slots, no iterator/pointer reliance). New
  `[[nodiscard]] Subscription ScopedSubscribe(EventType, Handler)`
  returns it; existing `Subscribe`/`Clear` unchanged (additive, all
  prior cases pass). Snapshot-before-dispatch + `shared_mutex` semantics
  and B1's `<mutex>` include preserved; mid-dispatch unsubscribe affects
  only later Publishes.
- **Guard:** `tests/test_eventbus_scoped.cpp` — out-of-scope unsubscribe,
  reconstructed B1/B2 heap-capture UAF proven safe, move-ownership / no
  double-unsubscribe, raw+scoped coexistence. Revert-verified: disabling
  the RAII unsubscribe fails all 4 cases.
- **Note:** B1/B2's `test_eventbus_isolation` boundary-`Clear()` listener
  can now be simplified/retired if every test subscriber adopts
  `ScopedSubscribe` — recommended, not done here (touches ~8 test files /
  out of this scope). `EventWiring.h`/`GameController` adoption deferred:
  the wiring is called from `src/GameController.cpp` (out of scope this
  pass).

---

## Content / quest-logic drift (found by dialog_lint at baseline)

### B3 [FIXED] Two dead quest flags — set by content, read by nothing
- **Symptom:** `dialog_lint.py docs/content/*.md` → 3 ERRORs:
  - `Flag_SawVictim_Ch1` — `chapter1.md:214`, annotated "Ch3 漣漪用".
  - `Flag_BoughtCoffeeForAuntie_Ch1` — `chapter4.md:275-276`, gates
    阿姨 direct-vs-indirect info.
- **Root cause:** the loader's `ScanFlag` will set these (it ignores the
  surrounding ``//``/back-ticks), but no `src/`/`include/` code ever
  reads them — an unwired ripple or a renamed/typo'd flag. The intended
  branch silently never happens.
- **Fix (decided with the GDD):**
  - `Flag_BoughtCoffeeForAuntie_Ch1` — **wired** (it is a named GDD
    LLM-schema flag, same §2.2 generosity model as 學長 熱咖啡, so it
    earns a real ripple). New Ch1 阿姨 (d) 請咖啡 `DialogChoice` seeds
    it (`setsFlag`, `// karma +5`); `ResolveOpenerSubState` now routes
    Ch4 `shop_auntie` to subState 0 (直接情報) when set, else subState
    3 (間接情報) — and `chapter4.md` (a) split into (a) direct / (d)
    indirect; `TryApplyCh4Ripple` grants the +3 (a)-route callback
    once (`Flag_Ch4Rippled_Auntie` guard), path-b like every other Ch4
    ripple (blockquote karma is doc-only, not parser-applied).
  - `Flag_SawVictim_Ch1` — **removed** (dead annotation; the 苦主
    Ch1→Ch3→Ch4 ripple is already driven by the live
    `Flag_PromisedVictim` (b)-承諾 path — `SawVictim` gated nothing
    and duplicated nothing). `chapter1.md` note now points at
    `Flag_PromisedVictim` / DialogOpener routing instead.
- **Guard:** `dialog_lint.py` exit 0 (0 errors); 3 new doctest cases —
  `test_ch4_ripple.cpp` (+3 once / state-gated) and
  `test_dialog_opener.cpp` (Ch1 (d) seeds flag +5 end-to-end via
  `ApplyDialogChoice`; Ch4 0-vs-3 routing). Revert-verified: reverting
  the production+content while keeping the tests fails all 3 cases and
  flips `dialog_lint` back to exit 1.

### B4 [FIXED] ~230 dialogue lines overflowed the 28-cell box
- **Symptom:** `dialog_lint.py` WARNs across all chapters (251): authored
  lines 30–54 full-width cells vs the 28-cell dialog box; in-game the
  text ran off the right edge / was clipped (no wrap, no pagination).
- **Root cause:** `DialogView.cpp` drew `CurrentLine()` as ONE unbounded
  `DrawText` at a fixed coord inside `Rect{20,320,760,110}`. The 28-cell
  rule lived only in `dialog_lint.py` (`MAX_CELLS`) — the renderer never
  enforced any width or height bound, so any long line overflowed.
- **Fix (renderer, not content):** new pure `DialogLayout`
  (`WrapToCells`/`Paginate`, East-Asian-Width cells == the linter's
  rule) + `DialogState` page cursor (`CurrentPageRows`/`HasMore`,
  `Advance()` turns a page before stepping the line) + `DialogView`
  draws one page with a "▼ more" affordance. CJK breaks on character
  boundaries, ASCII on spaces; a glyph wider than the box still cannot
  overflow. MVC purity kept: logic in View/DialogState, World pure,
  input unchanged in GameController.
- **Guard:** `tests/test_dialog_layout.cpp` (11 cases) incl. a
  content-wide case that drives EVERY authored line in all 9
  `docs/content/*.md` through the real engine and asserts no rendered
  row > 28 cells across all pages (683 assertions). Suite 247/247,
  0 skipped. Pixel-verify deferred (CJK font asset absent → tofu);
  verified via engine logic + a parity port + `dialog_lint.py` (now a
  renderer-contract assertion, 0 overflow warnings) as the documented
  fallback.

### B5 [FIXED] 21 content/grammar drift warnings (surfaced once B4 cleared the overflow noise)
- **Symptom:** after B4 + the lint scope fix, `dialog_lint.py
  docs/content/*.md` → 0 errors, **21 WARN** (down from 251):
  - **17×** `Flag_X not in 'Flag_x = true|false' form — flag not set by
    loader` — flags named in `>` prose / `*(若 …)*` annotations
    (e.g. `Flag_PromisedVictim` chapter1.md:226) that are actually set
    by engine code (DialogChoice / quest hooks), not a loader
    blockquote — *correct documentation* the lint heuristic over-flags
    (B3-class shape, but these flags ARE wired).
  - **4×** `quoted text inside a non-dialogue bullet — loader drops the
    line` — real **inline-conditional dialogue that never displays**
    (e.g. chapter2.md:139 `*（若 Flag_TookCursedUmbrella = true）*
    "……你今天感覺有點怪。"`). The DialogLoader grammar has no inline
    conditional; the intended reactive line is silently dropped.
- **Root cause:** two distinct defects the same WARN bucket conflated —
  (i) a *lint heuristic gap*: the flag-prose check did not distinguish a
  prose cross-reference of an engine-**wired** flag from a genuinely
  dead/unwired one, so every documentary mention of a real flag was a
  false WARN; (ii) a *genuine content bug*: 4 reactive beats authored as
  `*（若 Flag_X = true）* "…"` inline conditionals — a form the loader
  has no production for, so the reactive line loads as nothing and the
  player never sees it (the flag's narrative payoff is mute).
- **Fix:**
  - **Content (`chapter2.md`, +`chapter1.md`/`chapter4.md`):** the 4
    dropped inline-conditional beats re-authored as genuine flag-gated
    **separate substates**, routed (not inline-parsed): 學霸 圖書館初遇
    new `### (b)` for `Flag_TookCursedUmbrella` (default `### (a)` now
    explicitly the un-cursed常態; the old broadcast-notes sub folded
    into a `QuestFlagPickup`/`ShowMessage` note so `// karma +3` lands
    once, no double-trigger); 福利社阿姨 new `### (b)` for
    `Flag_BoughtUglyUmbrella` (greeting stays (a)); 苦主 `### (a)` split
    `Flag_PromisedVictim` 未承諾/承諾. `src/DialogOpener.cpp`
    `ResolveOpenerSubState` routes Ch2 to the reactive substate when its
    flag is set, else the default — same mechanism as B3's Ch4 routing.
  - **Lint (`dialog_lint.py`):** flag-prose heuristic refined — a prose
    cross-reference of a flag present in the engine whitelist
    (`--list-flags`) is no longer WARNed; only a genuinely unknown /
    unwired flag mentioned in prose still WARNs ("…mentioned in prose
    but not…", lint.py:299–307). Behaviour-neutral for real dead flags
    (still caught, as B3 was).
- **Also fixed here (lint scope, Cycle 1):** `dialog_lint.py` was
  linting `voice_bible.md`, a style guide never parsed by DialogLoader
  (`DialogSource.cpp:38-45`; its only `src` ref is the Font.h glyph
  list) → 57 false positives. Lint now mirrors the loader's file set
  and skips non-loaded docs (lint.py:362,385). *(logged in CHANGELOG.)*
- **Guard:** `tests/test_ch2_reactive_substates.cpp` (new) — asserts
  each re-authored reactive line actually loads **and** is reachable
  only under its gating flag (not from the default substate);
  **revert-verified** (reverting either the `chapter2.md` substate split
  or the DialogOpener Ch2 routing fails the CHECKs — the line is dropped
  / default (a) opens instead). Plus `dialog_lint.py docs/content/*.md`
  exit 0 / **0 WARN** as a standing gate.
- **Verification:** dialog_lint exit 0, 0 ERR, **0 WARN** across 8
  loaded files (was 21 WARN); `py_compile dialog_lint.py` OK.
  **Integrated re-gate landed on the main thread 2026-05-18T07:24Z**
  (after the harness-verb agent's `build_hv` quiesced — no build race):
  from-scratch build **0 project warnings** (`-Wall -Wextra -Wpedantic`,
  C++20); full suite **255/255 cases, 0 failed, 0 skipped** (1945
  assertions) — incl. `test_ch2_reactive_substates.cpp`'s in-suite
  revert-verify; `dialog_lint.py docs/content/*.md` exit 0 / 0 WARN.
  No caveats remaining — B5 fully closed.

---

## Playtest findings (filled by the team during iteration)

_(perception-driven defects from `.claude/out/*/state.jsonl` + shots)_

> **Cycle-2 perception correction (2026-05-18T07:59Z).** The first
> end-to-end harness exercise of the A/B/C spine (committed scripts
> `.claude/scripts/ending_{a,b,c}.txt`) proved **no ending is
> reachable** — all three soft-lock in Ch1 (verified on the main
> thread: ending_c ran 2× rc=0, 0 crash, byte-identical
> `sha256 baaffe62…4392`, `semester` never leaves `第一章 加退選`, no
> `結局`). Three engine defects below, **all verified from source on
> the main thread** (not agent-word). Prior cycles' "BUGLEDGER 0 open"
> was inaccurate: I3 was already cited by `test_scriptinput_plan.cpp`
> but never written here (latent undocumented OPEN). Replay determinism
> (convergence row 6) is sound; reachability (row 5) is RED.

> **Integrated re-gate 2026-05-18T08:31Z (main thread).** I3/I4/I5
> FIXED by two parallel agents (disjoint files) — verified on the
> integrated tree: from-scratch build **0 project warnings** (C++20,
> `-Wall -Wextra -Wpedantic`), full suite **261/261 cases, 0 failed,
> 0 skipped, 3191 assertions** (incl. all 6 new revert-verified
> doctests). I3 fix makes the game **human-winnable again** (manual
> walk-up + E opens dialog — doctest-proven). BUT re-running the
> committed A/B/C scripts still soft-locks in Ch1, **byte-identical**
> to pre-fix (sha A `cacdefba…`, B `ffa64daf…`, C `baaffe62…`) —
> root-caused to a NEW distinct harness-verb defect **I6** (below).
> So: row 4 = I3/I4/I5 FIXED + **I6 OPEN**; row 5 (scripted endings)
> still RED; row 6 determinism still sound.

### I3 [FIXED] interact-an-NPC was geometrically impossible (blocked all endings + human play)
- **Symptom:** a player walking up to a static NPC (harness `interact`
  OR a human pressing E) flush-stops but dialog NEVER opens; 6000-frame
  ending runs set no flag, never left Ch1. Cited as "BUGLEDGER I3" in
  `tests/test_scriptinput_plan.cpp:112-117` but never written.
- **Root cause (verified):** movement-blocker box and E-probe were
  effectively the same rect at the NPC origin — GameController built
  the blocker `Rect{npc, playerSize}` and E-probe `pHit{player,24,24}`;
  `Rect::Intersects` is strict so `physics::ResolveMove` halts the
  player EXACTLY flush (touching ≠ intersecting); a `!wander_` NPC
  could be touched but never strictly overlapped ⇒ the E-probe never
  collided ⇒ `OpenNpcDialog` never fired. `Flag_PromisedVictim` gates
  every umbrella claim ⇒ whole spine + 3 endings unreachable; same
  geometry hit a human pressing E.
- **Fix (Agent B):** explicit E-probe reach margin
  `constexpr float kInteractReach = 8.0f`; the probe is inflated to
  `Rect{px-8,py-8,40,40}` so a flush-blocked player still overlaps the
  NPC hitbox. The MOVEMENT collider (`frameColliders_`, ResolveMove,
  `Rect::Intersects` — `include/gfx/Rect.h` untouched) is byte-for-byte
  UNCHANGED: the player still cannot walk through an NPC; only the
  talk reach grows. 8px ≪ world NPC/item spacing. `src/GameController.cpp`
  only.
- **Guard:** `tests/test_i35_interact_vendor.cpp` — `I3: walking up to
  the Ch1 victim + E opens the dialog` (drives the real GameController
  loop, asserts `Dialog().Active()`/`NpcId()=="victim"`); companion
  `I3: player still cannot walk through a static NPC` (1200-frame shove,
  no pass-through). **Revert-verified:** restoring the 24×24 probe fails
  the dialog-open case (3 asserts); no-pass-through still passes.
  Integrated-gated 08:31Z (261/261, 0 warn).

### I4 [FIXED] ResolvePlan force-released classic WASD every frame (Cycle-2 regression)
- **Symptom:** classic-only scripts (no plan verbs) no longer moved the
  player through the harness; the exact `probe_ch1_gate.txt` that drove
  the player in an earlier cycle left it frozen every frame.
- **Root cause (verified):** `src/ScriptInput.cpp` `ResolvePlan` hit
  `if (planPc_ >= plan_.size()) { releaseMoveKeys(); return; }`; for a
  classic-only script `plan_` is empty so that ran every frame,
  `SynthUp`-ing W/A/S/D and undoing the held classic `down` edge
  `Advance()` (which runs first) had just applied. Comment + contract
  violated; no suite test covered this path so the 255-gate passed
  green over the regression.
- **Fix (Agent A):** `ResolvePlan` now returns at the very top when
  `plan_.empty()`, BEFORE `releaseMoveKeys()` is defined/called — a
  classic-only script's key state is never mutated by the plan
  resolver. Plan-bearing scripts byte-identical (early-return skipped;
  every verb / `finishStep` / exhausted-plan / `!world` path
  unchanged). Misleading comment rewritten. `src/ScriptInput.cpp` only.
- **Guard:** `tests/test_scriptinput_classic_move.cpp` — classic-only
  `0 down D`/`3 quit` driven 4 harness frames, asserts D stays
  `IsDown`/`IsPressed`(f0)/`!IsReleased`; + a plan-verb smoke case.
  **Revert-verified:** reverting the early-return fails case 1 (5
  asserts) while the plan case still passes. Integrated-gated 08:31Z.

### I5 [FIXED] Vendor::TryBuy had no runtime caller (Ending C + Ch2 spine dead)
- **Symptom:** no in-game purchase ever happened; Ending C's
  `Flag_BoughtUglyUmbrella` and the Ch2 clear (學霸 needs a bought
  EnergyDrink) were unreachable.
- **Root cause (verified):** a Vendor is built `NPC(...,npcId="")`, so
  GameController's `if(!id.empty()) OpenNpcDialog else o.Interact()`
  always routed it to `NPC::Interact` line-cycling, never
  `Vendor::TryBuy` (zero non-test callers).
- **Fix (Agent B):** virtual `GameObject::IsVendor()` (default false) /
  `Vendor::IsVendor()` (true) — same virtual-not-dynamic_cast
  convention as `BlocksMovement`/`NpcId`. GameController detects a
  Vendor, opens a buy-choice dialog (greeting + one `DialogChoice` per
  stock line, tagged with the inert sentinel context `"__vendor__"`),
  records a non-owning `Vendor* pendingVendor_`, and on confirm calls
  `Vendor::TryBuy(player, idx)` (idx captured before `Advance()` resets
  the cursor). ALL economy side-effects (DeductMoney, AddConsumable,
  ShowMessage + PickupAcquired EventBus events, soft-cap,
  `item.setsFlag`) stay inside `Vendor::TryBuy` untouched; the pinned
  `test_vendor` contract is unaffected. Ch2 EnergyDrink is thereby
  obtainable via `TryBuy → AddConsumable`, feeding
  `TryRescueBookworm::ConsumeOne` — no Vendor.cpp/EnergyDrink.cpp/
  Chapter2Quest.cpp change needed. `pendingVendor_` dropped the instant
  the menu is no longer the open conversation (vendor dialog freezes
  the sweep — never dangles). Files: `src/GameController.cpp`,
  `include/GameController.h`, `include/GameObject.h`, `include/Vendor.h`.
- **Guard:** `tests/test_i35_interact_vendor.cpp` — `I5: Vendor
  interaction routes to TryBuy` (money −100, `Flag_BoughtUglyUmbrella`,
  inventory +1, one `PickupAcquired`) and `I5: Ch2 progression — buy
  EnergyDrink, wake 學霸, Flag_Ch2Cleared` (full Ch2 clear via the real
  loop). **Revert-verified:** removing the `IsVendor()` branch fails
  both. Integrated-gated 08:31Z.

### I6 [FIXED] harness `interact` verb never pressed E (its arrival target was behind the movement blocker)
- **Symptom:** even with I3 FIXED, the committed `interact`-based
  ending scripts still soft-lock in Ch1 with state.jsonl
  **byte-identical to the pre-I3-fix runs** (the I3 GameController
  probe change is dead code along these timelines because E is never
  pressed). `reg_a1`: 6000 frames, **0** frames with `dialog.active`,
  player.x floor exactly **404** (the victim flush-stop).
- **Root cause (verified, main thread):** `src/ScriptInput.cpp`
  `case Verb::Interact` aimed at `tx = npos.x - kAdjacentDx` (`kAdjacentDx
  = 8`) and only pressed E once `AxisKeyToward` reported arrival
  (`k < 0`). Victim at x=380, blocker box 380..404; the player's 24px
  box flush-stops at **x=404**, but the target was **x=372** — *inside
  the movement-blocked region, physically unreachable*. So
  `AxisKeyToward(404,…,372,…)` returned "move left" forever, the verb
  stayed in the "still travelling" branch for its full `kInteractBudget`
  (4200 f) and the `SynthPress(E)` branch was never reached. Distinct
  from I3 (the GameController geometry, correctly fixed for human play);
  I6 was the harness-verb layer. Same family as I4 — a Cycle-2
  harness-verb-integration defect, latent until the spine was driven
  end-to-end.
- **Fix (specialist, `src/ScriptInput.cpp` only):** `Verb::Interact`
  now drives straight at the NPC **origin** and gates `SynthPress(E)`
  on the **same inflated-AABB overlap test the landed I3 fix uses** —
  `kInteractReach = 8.0f`, `pHit{pos.x-8,pos.y-8,40,40}`,
  `npc->CheckCollision(pHit)` (exactly mirroring
  `GameController.cpp:293-300`) — NOT on `AxisKeyToward<0` (an arrival
  test would reproduce the I6 soft-lock since the origin is itself
  flush-blocked ~24px away). Pure deterministic function of the two
  live positions. `kAdjacentDx` removed (now unused). Also a sanctioned
  harness-grammar extension `interact <label> <x> <y>`: umbrellas /
  `QuestFlagPickup` / Vendors have an empty `NpcId()`, so the plan had
  **no way to E-actuate any non-NPC world object** and the whole A/B/C
  spine (every ending must claim the TrueUmbrella) was unreachable —
  the coordinate form drives to (x,y) and taps E via the same I3-mirror
  geometry. Backward-compatible: `interact <npcId>` (no coords) is
  byte-unchanged. Confined to `Verb::Interact`/`Load()`, which only run
  for plan-bearing scripts ⇒ **cannot change normal play** (CLAUDE.md
  §5; classic scripts early-return at `plan_.empty()`).
- **Guard:** `tests/test_i6_interact_reach.cpp` — drives the **`interact`
  plan verb** through the real `ScriptInput`+`GameController` harness
  seam (Advance + ResolvePlan vs previous-frame snapshot) against the
  flush-blocking Ch1 苦主 and asserts `Dialog().Active()`/`npc=="victim"`
  + post-I3 reach geometry; plus a 2-run byte-identical determinism
  companion. **Revert-verified:** restoring the `npos.x-8` target +
  `AxisKeyToward<0` press gate fails `CHECK(o.dialogOpened)` (the I6
  soft-lock); restoring passes 2/2.
- **Test reconciliation (main thread):** `tests/test_scriptinput_plan.cpp`
  `interact victim` case asserted the **pre-I3** flush-exactly-24
  geometry (`|last.x-vx| <= 24`); it was green at the 08:31Z gate only
  *because* the I6 bug soft-locked the verb at the flush stop. Fixing
  I6 to mirror the landed I3 reach (dialog opens ~8px before flush,
  controller freezes the player during dialog ⇒ 27px) made `27<=24`
  fail. Reconciled to the post-I3/I6 reach band `<= 24+8` with rationale
  + a cross-reference to `test_i6_interact_reach.cpp` (which owns the
  dialog-opening guarantee). Legitimate post-design-change test update,
  not a code defect.
- **Integrated re-gate (main thread, this cycle):** from-scratch build
  **0 project warnings** C++20; full suite **263/263, 0 failed, 0
  skipped** (3206 asserts); `dialog_lint` exit 0; `ending_{a,b,c}.txt`
  via `playtest.sh` reach `結局 A` (8906f, karma 100) / `結局 B`
  (5734f, karma 28) / `結局 C` (5107f, karma 63), each **2× byte-
  identical** (`sha 3368e007…` / `4469969a…` / `223cc0ca…`,
  independently reproduced on the main thread, matching the
  specialist's reported shas), no soft-lock. NB: the first sweep
  falsely showed Ch1 soft-lock — caught as a stale `build/` binary
  (08:31 pre-I6); after `cmake --build build` the endings reached
  correctly. The game is now winnable to all three endings both by a
  human (I3) and by the autoplay harness (I6).

### I7 [FIXED] restored artifacts pre-convergence — Ch1 y≈1821 plaza soft-lock — code/tests `1f81d92` (Verdict B) · local harness ending_{a,b,c}.txt reconciled & 2× harness-verified
- **Symptom:** after restoring the uncommitted I3–I6 work (37-file
  patch → `a69b3da`) + TEAM-PLAN bundle, the autoplay harness
  soft-locks in Ch1. Main-thread verified (2026-05-18T14:32Z, build
  current, 0 proj-warn C++20): bare plan `goto 750 1860; goto 750
  1280` flush-stops dead at player **(749,1821)** for all remaining
  frames; `.claude/scripts/ending_c.txt` (5400f) never leaves
  「第一章 加退選」, no `結局`, karma 55. Unit suite **261/263, 2
  failed, 0 skipped** — `tests/test_scriptinput_plan.cpp:109` & `:178`
  (same `goto 750 1860\ngoto 750 1280` route, assert y≈1280, blocked
  at 1821).
- **Root cause (hypothesis, under investigation):** the patch carries
  the I6 code fix + `interact <label> <x> <y>` grammar
  (`src/ScriptInput.cpp:77-108`), but the committed
  `.claude/scripts/ending_{a,b,c}.txt` are the OLD pre-rewrite routes
  (consistent with the "Cycle-2 perception deliverable" note below:
  "blocked by I6; must be iterated until each reaches 結局"). The
  10:40Z LOOP report's "scripts rewritten / endings reach 結局" was
  never persisted into these restore artifacts. A 24×24 wall band
  (scattered plaza-market Vendors, commits 25b9389/67a2263; top
  ≈ y1797 ⇒ 24px player flush-stops at y≈1821) lies across the
  straight-up route. Open question: level/spawn defect (corridor
  genuinely un-routable) vs. stale routes only (production movement
  correct; routes must avoid the market).
- **Fix:** dispatched `playtest-bughunter` (background, disjoint
  `src/ include/ tests/`, own build dir) to root-cause A vs B, fix the
  git-tracked code/tests with a revert-verified doctest, and return
  corrected `ending_{a,b,c}.txt` route text (`.claude/` is
  agent-forbidden ⇒ main thread writes it) + integrated re-gate on the
  main thread.
- **Guard:** the 2 `test_scriptinput_plan.cpp` cases + a new
  reachability doctest must go green, and `ending_{a,b,c}.txt` must
  each reach `結局 A/B/C` 2× byte-identical with no soft-lock, before
  I7 → [FIXED].
- **Verdict — B (stale routes, not the engine), confirmed.** The
  shipped `collision_mask.png` (versioned `964b4ee`, AFTER the harness
  scripts + plan tests were authored against an all-walkable world)
  bakes a continuous E–W south-campus wall at **y≈1761–1819** whose
  ONLY gap is the column **x≈880–1042**. Every committed `goto` drove
  straight up a walled column ⇒ flush-stop at y≈1821, byte-identical
  every run. `goto` is a pure axis driver by design, not a path-finder;
  the campus stays traversable through the gap. No level/engine defect.
- **Resolution — code/tests (DONE, gated, pushed).** Commit `1f81d92`
  on `claude/restore-project-state-6GSBZ` (HEAD == origin, 0 ahead/0
  behind): `test_scriptinput_plan.cpp` — the 2 stale cases rerouted
  through the x≈1041 gap, the goto+E case now also asserts
  `Flag_HasTrueUmbrella` so it genuinely exercises drive+E→beClaimed;
  `test_ch1_spine_reachable.cpp` (new, 198 ln) pins the mask gap
  geometry (graceful skip on asset-less clone) + the minimal
  Ch1→Interlude→Ch2 spine on the real mask. Gate green: clean C++20
  build 0 proj-warn · **ctest 266/266, 3340/3340 assertions, 0
  failed/0 skipped** (was 261/263, 2 failed).
- **Resolution — harness scripts (DONE, harness-verified, I7 CLOSED).**
  `.claude/scripts/ending_{a,b,c}.txt` rerouted through the gap by
  `playtest-bughunter` and landed by the main thread (`.claude/` is
  agent-forbidden). Game semantics preserved exactly — the verb-only
  sequence (`interact/choose/advance/wait/quit`, targets/indices/
  counts/order) is byte-identical old↔new (132 / 51 / 52 verbs for
  a / b / c); ONLY `goto` legs + the now-corrected header comments
  changed; no forbidden brand/model strings. **Independently re-run on
  the REAL `playtest.sh` harness (main thread, not the agent's sim):**
  · `ending_a` → `結局 A`, 7490 state lines, karma 100 · `ending_b` →
  `結局 B`, 4834 lines, karma 28 · `ending_c` → `結局 C`, 4142 lines,
  karma 63. Each self-terminates on its own `quit` far below the 12000
  maxframes (no soft-lock; semester progressed monotonically
  Ch1→幕間→Ch2→幕間→Ch3→幕間→Ch4→結局). Determinism (CLAUDE.md §4): each
  script run 2× → `state.jsonl` byte-identical (`cmp` clean; sha
  a=`d2feb9e8…` b=`40017cfe…` c=`2e455ca0…`, matching the agent's
  independently-measured shas).
- **Routing model (for future mask edits / script authors).** The
  shipped `collision_mask.png` has exactly one road↔north passage —
  the gap column x≈880–1042 (clear y≈1240–1990); the south road
  y≈1830–1920 is the only full-width E–W band. North of the wall there
  is NO thick E–W corridor reaching the far east — only thin (~16–30px)
  winding diagonal slots — so a long Manhattan `goto` accumulates the
  slot's curvature and soft-locks (EPS-2 arrival drift compounds).
  Every north-of-wall target is therefore reached as a chain of short
  hops physics-certified against the real `ScriptInput` axis driver
  (3 px/frame, X-then-Y, flush-stop on mask + that chapter's roster) —
  same gap model `tests/test_ch1_spine_reachable.cpp` pins. The
  Interlude entry repositions the player to {500,1500} (north of the
  wall; x=500 is walled), so every IL exit routes back DOWN through the
  gap to the south band — never straight down (the old scripts'
  `goto … 1965` was a latent westbound soft-lock too). The single mask
  applies to all 4 chapters; only rosters differ.

---

## Cycle 3 — experience pass (perceive→assess→fix, gated 2026-05-19)

3 read-only assessment agents (gfx/UX, balance/quest, bughunter) swept
84 shots + the full A/B/C `state.jsonl` + 3 stuck-probes. Highest §7
tier CLEAR: **no crash, no soft-lock, no flag pollution, no event
anomaly, bounds sane**; determinism triple-re-confirmed (matches the I7
shas). All findings are experience/balance/content tier.

### I8 [RESOLVED — LETHAL, via the Cycle-4 drain rule] rain-survival meter was dead code
- **Symptom:** `player.rain` 0 for all 16 466 trace frames + a 3000-f
  idle probe. The GDD core "雨量計→正門" survival loop never ran.
- **Root cause (triple-confirmed):** `Player::ApplyRain`/`RespawnAtGate`
  had **zero** callers in `src/ include/` (only def/decl + a unit test).
  Same family as I5 (designed-but-unwired). STRICT_REVIEW.md M2/M3's
  "ApplyRain 每幀 Publish" was fiction that masked it.
- **Fix:** `ApplyRain(float dt, bool lethal = true)`;
  `GameController::Update()` ticks `ApplyRain(dt, false)` once/frame
  OUTDOORS (semester ∉ {Interlude, Ending_*} ∧ `CurrentBuildingName()`
  empty), after the dialog/Tab freeze early-returns. `lethal=false`
  accumulates+clamps but skips `RespawnAtGate`. Default `true` ⇒
  `test_player_core.cpp` byte-unchanged.
- **DEFERRED (decision: user chose conservative wiring):** the lethal
  ≥100→正門 teleport + half-day AND any movement-speed/slow penalty —
  both perturb the deterministic `goto` ending scripts. A future
  dedicated cycle owns lethal rain + script re-route + economy/pacing
  re-tune (F5/F7 are downstream of it).
- **Cycle 4 perceive (2026-05-19, lethal flip + reverted):** flipped
  GC tick to `lethal=true`, rebuilt 0-warn, ran A/B/C. Result:
  **B & C byte-identical to baseline, 0 teleports, still 結局 B/C**
  (rain peaked 71.5 / 89.3, never breached 100 — lethal is a no-op
  for them). **A: hard soft-lock in Ch1** (12000-frame cap, sem stuck
  `第一章 加退選`, **10 300 teleports**). Root cause is **design,
  not a stale route (NOT the I7 pattern):** the rain meter is
  **monotonic — no drain path** (`resetRainMeter()` is called only by
  `RespawnAtGate`; umbrella/shelter only *pause* accrual). Ending A
  structurally requires the full Ch1 chain
  victim→form→ta→suit_senior→shop_auntie→trueumb spanning the whole
  map (ta x=1748 ↔ Ch1 TrueUmbrella `World.cpp:28` {320,1280},
  the Ch1-clear trigger) — first teleport at frame ~1700 (~28 s
  outdoor) vs the 20 s (100÷5) umbrella-less budget. No script can be
  authored to survive it ⇒ **lethal rain needs a rain-drain rule
  (or a Ch1 restructure / benign early umbrella), not just a
  re-route.** Reverted to the gated-green conservative state
  (`git diff` empty vs 21ed846); escalated the drain-model design
  fork to the human (multi-valued, like the conservative/lethal
  fork). Concrete numbers pinned here so this is not naively retried.
- **Cycle 4 RESOLUTION (user chose "add a drain rule", 2026-05-19):**
  added `Player::DrainRain(float dt) noexcept` (−10 u/s, clamp[0,100],
  no teleport). GC tick rewritten: market/endings skipped; else
  SHELTERED (umbrella OR `CurrentBuildingName()` non-empty) ⇒
  DrainRain, EXPOSED ⇒ `ApplyRain(dt, lethal=true)` (≥100→RespawnAt
  Gate now LIVE). Result: the existing ending scripts already shelter
  enough (in-building dialogs + umbrella claims) that net rain peaks
  **38.4/35.4/67.5**, never hits 100 ⇒ **0 teleports, ZERO script
  re-authoring**, all three reach `結局 A/B/C` 2× byte-identical, no
  soft-lock, line counts = baseline (7490/4834/4142). Lethal is
  genuinely armed (regression proves the gate fires when truly
  over-exposed); competent play simply never trips it — the GDD
  survival pillar, finally real. Tunables: accrual 5 / drain 10 u/s.
- **Guard:** `tests/test_rain_survival.cpp` (renamed from
  `test_rain_exposure_conservative.cpp`; 4 cases, revert-verified:
  DrainRain unit −10/clamp0/no-teleport · GC outdoor-umbrella-less
  accrues then the LETHAL gate fires (落湯雞 msg + meter reset, not
  pinned) · equipping an umbrella makes GC DRAIN to 0 · market = no
  tick). `test_player_core.cpp` one-arg ApplyRain case untouched
  (lethal defaults true ⇒ byte-unchanged).

### F2 [FIXED] CursedUmbrella karma penalty −50 exceeded the locked −15/−30 scale
- **Symptom:** single cursed claim → K−50 (GDD/SCRIPT_HANDOFF cap big
  events at −15/−30; 67 % over).
- **Fix:** `include/CursedUmbrella.h:9` `karmaPenalty_` 50→30; stale
  `CursedUmbrella.cpp` event text `"Karma -50"`→`"-30"`. Ending B
  reachability unchanged (`EndingGate` routes on
  `Flag_TookCursedUmbrella`, set unconditionally by `beClaimed`).
- **Test reconciliation (main thread, the I6 precedent):** two
  pre-existing tests hard-coded `before-50` —
  `tests/test_ripple_seed_flags.cpp:39,43` and
  `tests/test_factory.cpp:49` updated to `-30`; intent (seed-flag +
  single-penalty + idempotent) preserved, magnitude only. Caught by
  the integrated re-gate (α only saw the ripple test) — trust-but-
  verify working as designed.
- **Guard:** `tests/test_cursed_penalty_scale.cpp` (revert-verified:
  exactly −30, idempotent, composes linearly).

### F3a [RESOLVED — doc-only] Ending B `karma<0` clause unreachable in non-cursed play
- **Symptom:** karma clamp[-100,100] from 50; max non-cursed negative
  path ≈ −38 ⇒ `EndingGate.cpp:34 || GetKarma()<0` never fires without
  the cursed flag. GDD §陸 "karma<0" path was fiction.
- **Resolution:** one clarifying sentence in `遊戲企劃與敘事架構.md`
  (B is in practice `Flag_TookCursedUmbrella`-driven; `karma<0`
  retained as a defensive lower bound). `EndingGate.cpp` UNCHANGED.

### V1 [FIXED] "▼ more" pagination affordance rendered as tofu `?`
- **Symptom:** dialog box bottom-right `?` not ▼ (c3_b f4800 shot).
- **Root cause:** `DialogView.cpp:50` emits U+25BC but the atlas
  (`Font.h CollectCodepoints` = ASCII + content-md + `UiLiteralChars`)
  never included it → raylib no-glyph `?`.
- **Fix:** added U+25BC (+ V3 caption ideographs / CJK quotes absent
  from content) to `UiLiteralChars()`. No `DialogView.cpp` change
  needed. **Guard:** `tests/test_font_ui_glyphs.cpp` (revert-verified).

### V2 [FIXED] top-left karma/umbrella HUD illegible
- **Symptom:** WASD/karma/umbrella/chapter in DarkGray/Blue on the
  bright worldmap, no panel (c3_c f200 shot).
- **Fix:** `View.cpp` — translucent `Color{20,22,30,185}` panel +
  bright text (the proven objective-panel idiom) + a new `rain: NN%`
  readout (White→Gold≥60→Red≥85) + a pure-render screen-edge rain
  vignette (rm≥60 α45 / ≥85 α90). MVC-safe (reads `GetRainMeter()`/
  `GetKarma()`/`HasUmbrella()` only; no sim/determinism effect).

### V3 [FIXED] Ending A/B 字卡 were placeholders + off-centre
- **Symptom:** A/B caption = `（結局字卡待 Phase 2 接入）`; only C real.
  Title/caption x hardcoded (−60/−220), not measured.
- **Fix:** `EndingView.cpp` — real A `「雨過天晴，傘還在你手上。」` / B
  `「你成為了你曾經最討厭的那種人」` (grey-tinted per GDD §陸)
  captions; `CellWidth`-measured horizontal centring (the only
  font-independent measure helper — `IRenderer` has no `MeasureText`).
  Still 1 DrawRect + 2 DrawText ⇒ `test_ending_card_render.cpp` green.
- **Visual re-capture: CONFIRMED** (A f7488 / B f4830 / C f4140) — all
  three are clean centred takeovers, B grey-tinted, glyphs no-tofu.
  ⚠ Perception note for future cycles: `View.cpp:61` ramps
  `endingAlpha_ += 1/60`/frame ⇒ a **60-frame** fade, but the
  `ending_{a,b,c}.txt` scripts `quit` only ~28 f after the gate fires,
  so harness shots of the card are always mid-fade (the previous
  gameplay framebuffer bleeds through at low α). Sampling A at fire+8
  vs B/C at fire+24/26 looked like an "A renders over the world"
  defect — it is NOT (single `DrawEndingCard` path, IsEndingState
  early-returns the world). Cosmetic-only; a human waits the full 1 s
  and sees a fully opaque card. If wanted, a future polish: extend the
  three scripts' post-結局 hold to ≥60 f (harness-script only, no
  game code) — logged, not actioned (not a defect/regression).

### Stale review docs (bughunter-verified false vs current code)
`docs/STRICT_REVIEW.md` M1 ("nodiscard 0處"→74), C2 ("EventBus 未保護"
→`shared_mutex`'d), M2/M3/L253 ("ApplyRain 每幀 Publish"→was no-caller,
now conservatively wired), "main.cpp god"→67-line root;
`docs/SOLID_REVIEW.md` #2/#3 reference deleted code;
`docs/STRICT_REVIEW_R3.md` C-1 RespawnAtGate-UAF is hypothetical
(never called, repositions not deactivates). Annotated SUPERSEDED in
each doc (CLAUDE.md §2 stale-baseline tier; not rewritten).

---

## Cycle-2 perception deliverable

- Committed progression scripts `.claude/scripts/ending_{a,b,c}.txt`
  (robust plan-verbs, fully commented) authored to the authoritative
  `EndingGate` design. Each runs 2× rc=0, **replay byte-identical**
  (sha256 A `cacdefba…23ce`, B `ffa64daf…4359`, C `baaffe62…4392`;
  re-verified on the main thread 08:31Z, unchanged after the I3/I4/I5
  fixes — because **I6** stops the `interact` verb from ever pressing
  E). Correct-by-design but currently blocked by I6; once I6 is fixed
  they must be iterated until each reaches `結局 A/B/C` deterministically
  with no soft-lock. *Revert:* delete the three files.

---

## Cycle 6 — live-play soft-lock + §5 guard (2026-05-19, lead-integrated)

The human disputed the Cycle-5 "converged" verdict. Live play of all 8
scripts (game-team-lead) surfaced two real defects; both fixed with
revert-verified regressions, then integrated with a parallel UI worktree
(disjoint files → conflict-free merge `b26aa9a`). Lead re-gated GREEN on
the integrated tree: bare `cmake -B build`, **0 project warnings**,
suite **280/280 / 3601 assertions / 0 fail / 0 skip**, `dialog_lint`
exit 0, harness bypass of the new title/select confirmed (player
in-game from frame 1, semester monotonic 第一章→幕間→第二章→第三章…).

### L1 [FIXED] Ch4 助教 finale soft-lock — game unwinnable (CLAUDE.md §5)
- **Symptom:** the honest `ending_a` spine but the 助教 (d) 結算 picks
  choice 1 (質問／強硬索回) instead of 0 (體諒): 8071 trace frames,
  **0 frames in any 結局**, final `sem=第四章 期末`, karma 95,
  `Flag_TaFinaleChoiceMade=true`, `Flag_ConsoledTA=false` — stuck
  forever. Repro `/tmp/probe_ta_hard.txt`.
- **Root cause (verified):** `src/GameController.cpp:260-262` locks the
  finale menu on any confirmed choice; `src/EndingGate.cpp` had **no
  terminal branch** for the cold-finale path (or 體諒 with karma≤80) ⇒
  an honest player falls through A/B/C and is soft-locked. Violates
  CLAUDE.md §5 ("do not make the game unwinnable").
- **Fix (game-team-lead, `d3b25ab`):** `src/EndingGate.cpp:32-70` — the
  gate is now **TOTAL** once `Flag_TaFinaleChoiceMade` is set: cold
  finale → Ending B (thematically correct per GDD §陸 B 字卡);
  `Flag_TaFinaleChoiceMade` alone → Ending C (GDD 破財消災 "Normal"
  default). Strictly gated on the finale flag so **pre-finale Ch4
  free-roam is byte-unchanged**; A→B→C precedence preserved.
- **Guard:** `tests/test_ending_gate.cpp` — `ending gate: 助教 finale
  is TOTAL — no fall-through soft-lock` (4 subcases) + `ending gate:
  pre-finale Ch4 free-roam is byte-unchanged` (2). **Revert-verified:**
  against pre-fix code the TOTAL case fails (`4==7`, stuck in
  Chapter4_Finals).

### L2 [FIXED] §5 red-line — True/Fragile Umbrella beClaimed not idempotent
- **Symptom:** `src/TrueUmbrella.cpp` & `src/FragileUmbrella.cpp`
  `beClaimed` had **no `isActive_` guard** (unlike `CursedUmbrella.cpp:7`
  / `ProfessorTrapUmbrella.cpp:7`). A 2nd call re-publishes
  `UmbrellaClaimed`; the Ch1/Ch3 `EventWiring` sibling-if advances the
  semester on that event ⇒ latent double semester-transition (`OnPickup`
  is a 2nd entry point not covered by the caller's active-filter).
- **Fix (game-team-lead, `4c36337`):** `src/TrueUmbrella.cpp:7` &
  `src/FragileUmbrella.cpp:7` — `if (!isActive_) return;`. Restores the
  CLAUDE.md §5-mandated pickup idempotency guard.
- **Guard:** `tests/test_ripple_seed_flags.cpp` —
  `TrueUmbrella::beClaimed is idempotent` +
  `FragileUmbrella::beClaimed is idempotent`. **Revert-verified:**
  pre-fix `claimed 2!=1`.

### UI shell (agent B, worktree `6d9ca97` → merged `b26aa9a`)
Title screen · 5 non-binary personas (runtime colour-tint over
already-shipped Pipoya sheets — **no new assets**, CLAUDE.md §5) ·
always-on money HUD (`金幣: N 元`) · in-game top-right pause menu
(繼續／重新開始／離開, sim frozen while open; Restart rebuilds
{World,View,GameController}). **Harness bypass preserved** — when
`UMBRELLA_SCRIPT` is set the title + select are skipped
(`UMBRELLA_SPRITE`); lead-verified the player is in gameplay from
frame 1, all `.claude/scripts/*` unchanged. **Guard:**
`tests/test_restart_safety.cpp` — state reset (karma/money/flags) + no
EventBus dangle/double-subscribe (BUGLEDGER B2/H1). Zero file overlap
with L1/L2.

### Environment incident (NO data lost — recorded for the architecture-reviewer)
The worktree agent's bash cwd was unstable: `cd /home/user/ultraplan
-sync`-prefixed commands ran in the **shared main tree** (a sibling
worktree sharing `.git`), so its first commit landed on
`claude/restore-project-state-6GSBZ`; it recovered via `git stash` +
`git reset --soft 6bf5a5e` + `git restore --source=6bf5a5e`, which
**transiently** reverted the logic agent's in-flight `EndingGate.cpp`.
Lead read-only forensics: **origin clean** (still `6bf5a5e`, no
force-push, no pollution), main ref correct, both agents' work intact
and committed (`d3b25ab`/`4c36337`/`6d9ca97`), triple backup existed
(dangling stash `4ed22344` `WIP on …: EndingGate.cpp +35` / blob
`25f4f01`). Net: no loss. **Decision: no further parallel *worktree*
agents in this environment** — its cwd isolation for worktree-scoped
agents is unreliable; remaining work proceeds single-writer.

---

## Cycle 7 — requirements #4–#10 (single-writer; R-numbers track the
user's requirement numbers for traceability)

### R4 [FIXED] vendor forces a purchase — no 「不買」 decline path
- **Symptom:** opening a stall's buy menu offered only purchasable
  stock; there was no way to leave without spending — every entry
  mutated economy/flags (user requirement #4).
- **Root cause:** `OpenVendorMenu` built the choice list from stock
  only; no terminal decline option, and an empty stock opened no menu.
- **Fix (`1df4a95`):** append a trailing `先不買，謝謝` choice (always
  open a menu, even on empty stock); the decline index (== stock size)
  closes the dialog with **zero** mutation (no `DeductMoney` /
  `AddConsumable` / `setsFlag` / `PickupAcquired` / event). Appended
  LAST so all stock indices and the deterministic ending scripts
  (choose 0) are byte-unaffected; `Vendor::TryBuy(stockIdx)` contract
  unchanged.
- **Guard:** `tests/test_vendor_decline.cpp` (real GC loop; decline ⇒
  state delta zero). **Revert-verified.** `test_i35` reconciled (1→2).

### R10 [FIXED] building-name HUD renders `?` (missing CJK atlas glyphs)
- **Symptom:** `View`'s `Inside: <name>` HUD line showed `?` for 7
  building names.
- **Root cause:** 8 ideographs (井/仁/勇/塘/夫/志/泳/雩) used by
  `Buildings.h` occur in no `docs/content` file, so they were never
  added to the font atlas ⇒ raylib draws its no-glyph `?`.
- **Fix (`1df4a95`):** bake the full 56-glyph `Buildings.h` name set
  into `Font.h UiLiteralChars()` (same atlas mechanism as the V1 ▼
  fix).
- **Guard:** `tests/test_font_ui_glyphs.cpp` drives the real
  `Buildings.h` table (revert-verified; future renames covered).

### R6 [FIXED] every market stall renders the same person
- **Symptom:** on a clean clone all ten market stalls showed one
  identical 攤主 sprite (user requirement #6: must be different people).
- **Root cause:** the spawn loop passed a single `shop_auntie.png` to
  every stall.
- **Fix (`8bc9c9f`):** new pure `VendorSprite.h` selector keys off each
  stall's unique 攤主 and assigns a distinct curated fallback per spawn
  index — ships-only sprites, **no new art** (§5).
- **Guard:** `tests/test_vendor_centred_cluster.cpp` (pairwise-distinct
  selector). **Revert-verified.** `test_vendor_loader.cpp` reconciled
  (distinct-X → pairwise-distinct, the real intent).

### R7 [FIXED] market stalls scattered — not at 羅馬廣場 centre
- **Symptom:** stalls were spread off the plaza (user requirement #7:
  all at the centre of 羅馬廣場).
- **Root cause:** legacy per-stall positions predate the plaza layout.
- **Fix (`8bc9c9f`):** gather all ten into a verified-walkable bullseye
  at plaza centre **(1088,960)** — centre + r42 + r78 rings, every pair
  > 35 px (no overlap), max r78 ≪ the verified r100 walkable box.
- **Guard:** `tests/test_vendor_centred_cluster.cpp` (geometry pinned)
  + `test_spawn_reachability`. **Revert-verified.**

### R9 [FIXED] the four umbrellas are visually indistinguishable; no
in-game help
- **Symptom:** all umbrellas drew the same glyph/tint; nothing
  explained the game (user requirement #9: distinct looks + 遊戲說明 +
  a 說明 menu item).
- **Root cause:** a single shared umbrella render path; no help screen
  or help menu entry existed.
- **Fix (`8bc9c9f`):** data-driven `UmbrellaStyle` ⇒ 4 distinct rect
  silhouettes + bold separated tints (cyan domed / grey broken / amber
  spiked / violet drooping); shared `GameHelp.h` drives both a title
  遊戲說明 item and a pause-menu 說明 item (menu now
  繼續/說明/重新開始/離開) so they never drift; help glyphs baked into
  the atlas. MVC-pure (render/UI only; harness still bypasses menus).
- **Guard:** `tests/test_umbrella_render.cpp` (rewritten — distinct
  fingerprints/tints) + `tests/test_menu_help.cpp` (real GC loop).
  **Revert-verified.**

### R5 [FIXED] rain pressure vanished after Chapter 1
- **Symptom:** rain only mattered in Ch1; Ch3 was literally **0.0**
  every frame (user requirement #5: pressure every chapter).
- **Root cause:** holding the Ch1 TrueUmbrella gave *permanent*
  immunity — `ApplyRain` self-noops with an umbrella **and** the GC
  treated "has umbrella" as full shelter ⇒ `DrainRain`. Net Ch2≈2.8,
  Ch3≈0.0, Ch4≈2.4.
- **Fix (`a5148f6`):** new `Player::ApplyRainSheltered` (+1.5 u/s ≈
  30 % of exposed +5, clamp [0,100], lethal-armed) + a 3-way GC tick:
  inside a building → `DrainRain` (−10); outdoors + umbrella →
  `ApplyRainSheltered`; outdoors no umbrella → `ApplyRain` (+5,
  **byte-unchanged**). Umbrella buys time, not immunity. Per-chapter
  max now Ch1≈38 / Ch2≈52–81 / Ch3≈49–62 / Ch4≈57–66; competent play
  never trips 100; all ending scripts 0 teleports; Ch1 byte-identical.
- **Guard:** `tests/test_rain_survival.cpp` (3-way contract + sheltered
  unit). **Revert-verified.** Rate tunable in `ApplyRainSheltered`.

### #8 [VERIFIED — NO DEFECT] Ch2/3/4 plot-driving NPCs present
- **Finding:** a full-spine trace shows each chapter has spawned,
  reachable, firing quest-drivers: Ch2 學霸 + 圖書館管理員; Ch3
  香腸→大聲公→學姊 chain; Ch4 助教 finale. The perceived gap was the
  prior-cycle I3/I5/I6 reachability bugs (already fixed). No code
  change — recorded so later cycles do not re-investigate.

### Cycle-7 gate (lead-verified, independently re-run)
`a5148f6 == origin`, tree clean · bare `cmake -B build` OK · **0
project warnings** (C++20) · suite **287/287, 4235 assertions, 0
fail/0 skip** · `dialog_lint` exit 0 · endings A/B/C reach 結局 A/B/C
at **7490/4834/4142** = baseline, **0 rain-teleports**, winnable;
ending C 2× byte-identical, integrated ending A 2× byte-identical
(`sha256 3eb4ab40…`); lead independent `ending_a` re-run rc=0,
7490 lines, `last_sem=結局 A`. Single-writer throughout — no repeat of
the Cycle-6 worktree-cwd incident.

---

## Cycle 8 — narrative conformance audit (lead-integrated, single-writer)

### N1 [FIXED] Flag_ScoldedSenior wired by code, never set by content — entire GDD-named cold-senior arc dead
- **Symptom:** `chapter1.md` 西裝學長 had (a)/(b)/(c)/(d) with both (c)
  and (d) explicitly annotating `Flag_ScoldedSenior = false`; **no
  substate set it `= true`**. Yet `src/DialogOpener.cpp:101` (Ch2
  suit_senior → cold (c)), `src/Chapter2Quest.cpp:66` (cold −3 ripple),
  `chapter4.md:82/88/405` (學長不出場 / context unavailable arc), and
  `voice_bible.md:51` (Interlude 側身走開) ALL branch on
  `Flag_ScoldedSenior = true`. The GDD-named §伍 選項 B 「憤怒地斥責
  學長，直接把傘搶走」 cross-chapter cold-senior arc was permanently
  unreachable dead narrative content.
- **Root cause:** content drift — `(b)` was authored as an inert "拒絕"
  stub (`karma +0`, no flag), and no later content edit added the
  GDD's 選項 B path. The codebase grew the reads (cold-senior content
  in chapters 2/3/4) on the assumption a Ch1 setter existed.
- **Fix (`496a771`, content-only):** `chapter1.md` (b) re-authored as
  the GDD §伍 選項 B 「玩家憤怒斥責，奪回雨傘」 — `karma −5`
  (confrontational tier, mirrors (c) `−5` 受騙取陷阱傘),
  `Flag_ScoldedSenior = true`. Re-authored (b) — NOT appended (e) —
  because `src/DialogLoader.cpp:83` hard-caps substate letters at
  `'a'..'d'` (CLAUDE.md §6 literal). Only
  `.claude/scripts/ending_a.txt` interacts with suit_senior (picks
  `choose 2` = (d) HelpedSenior); endings A/B/C state.jsonl byte-counts
  and SHAs preserved. **Tradeoff acknowledged:** the original (b)
  peaceful-decline beat is no longer reachable; user accepted vs the
  loader-cap engineering cost.
- **Guard:** `tests/test_dialog_opener.cpp` — 2 new F2 cases (choice
  index 0 sets the flag via `ApplyDialogChoice`; `ResolveOpenerSubState`
  Ch2 suit_senior routes to (c) cold when flag set) +
  `tests/test_loadchapter_chapter1.cpp` parity update. **Revert-
  verified:** reverting the `chapter1.md` (b) edit (alone, tests kept)
  fails all 3 new/updated CHECKs; restoring → 289/289.

### N2 [FIXED] Flag_KnowsUglyUmbrella inert seed annotation (B3 dead-flag class)
- **Symptom:** `chapter1.md` (c) (玩家購買醜綠傘後) annotated
  `// Flag_KnowsUglyUmbrella = true` as an "Ending C 路徑種子";
  `src/EndingGate.cpp:12` comment said the real Ending-C trigger is the
  Ch4 集英樓 Vendor (sets `Flag_BoughtUglyUmbrella`). `grep` found no
  `HasFlag("Flag_KnowsUglyUmbrella")` in any `src/`/`include/` source —
  the flag was set by content, read by nothing. Same shape as the B3
  dead-flag class (`Flag_SawVictim_Ch1`).
- **Root cause:** vestigial annotation from an earlier design iteration
  where Ending C had two trigger paths (Ch1 seed-then-confirm + Ch4
  direct purchase). Iteration consolidated to the Ch4 path only; the
  Ch1 annotation was left as inert documentation.
- **Fix (`b33db2b`, content + small cleanup):** `chapter1.md` (c)
  annotation replaced with a prose note documenting the real Ending-C
  trigger; `src/Harness.cpp` `KnownFlags()` state.jsonl dump whitelist
  drops the flag (it never appeared in any trace — confirmed by `grep`
  over `.claude/out/*/state.jsonl`); `src/EndingGate.cpp` header
  rewritten to match; `docs/SCRIPT_HANDOFF.md` "Ending C 觸發點"
  repointed to make the Ch4 Vendor the only trigger. The `dialog_lint`
  engine-flag whitelist (dynamically scanned from `Flag_*` tokens in
  `src/include/`) no longer lists the flag — verified post-fix with
  `--list-flags`. Mirror of B3's `Flag_SawVictim_Ch1` removal, applied
  to the same dead-flag class.
- **Guard:** `tests/test_dialog_opener.cpp` (c) choice now expects
  `setsFlag == ""` (was `"Flag_KnowsUglyUmbrella"`);
  `tests/test_ending_gate.cpp` seed-flag test rewritten to pin the
  absence-of-flag invariant. **Revert-verified:** reverting the
  production changes (`chapter1.md` + `Harness.cpp` + `EndingGate.cpp`
  + `SCRIPT_HANDOFF.md`, tests kept) fails the (c) choice `setsFlag`
  check.

### Cycle-8 gate (lead-verified, independently re-run)
`b33db2b == origin`, tree clean · bare `cmake -B build` OK · **0
project warnings** (C++20) · suite **289/289, 4254 assertions, 0 fail
/ 0 skip** · `dialog_lint` exit 0 (+ `--strict` clean) ·
`dialog_lint.py --list-flags` no longer lists
`Flag_KnowsUglyUmbrella` · endings A/B/C reach 結局 A/B/C at
**7490/4834/4142** = baseline, 0 rain-teleports, 2× byte-identical
(`572503d4…` / `f498d4a4…` / `029cdad3…`). Single-writer throughout.
