# Audit — docs/STRICT_REVIEW_R3.md

**Overview (≤3 sentences):** The R3 review was authored against the 3-commit batch `4a9f661 / 0007686 / 5ee0258`; the document has a SUPERSEDED banner (line 1–8) admitting C-1's Player* UAF was hypothetical, and Cycle 3's BUGLEDGER tail already flags the doc as stale (BUGLEDGER l.606). Of the 28 documented R3 elements, **most CRITICAL hazards are already fixed** in code (CR-1 comment, CR-2 player-cache, CR-3 lifetime, CR-4 wiring split, single-reviewer items 1/4, SPRINT #1–7); the only genuine open gaps are CR-5 (`main()` still a 7-axis composition root, intentionally), Factory-method rename/refactor (DIP D-C2), SPRINT #8 (PlayerAnimator extraction), and the optional Perf #10 compile-time mutex flag.

## Per-element annotations

- **SUPERSEDED banner (l.1–8)** — Self-disclaims C-1 as hypothetical. **[Yes]** — `docs/STRICT_REVIEW_R3.md:1-8` (CLAUDE.md §2 stale-baseline tier). **[No conflict]** — Stale-doc-only by design.
- **TL;DR #1 EventBus comment lies (l.34-37)** — "concurrent dispatch safe" is wrong; fix the comment, optionally `kEventBusThreadSafe` flag. **[Yes]** — `include/EventBus.h:97-107` rewritten: comment now states "the handler list only — not handler bodies; do not publish off the main thread". **[No]**.
- **TL;DR #2 Player* survives erase (l.38-42)** — replace with `weak_ptr` or handle. **[Yes — alternative form]** — `src/GameController.cpp:529,537` uses `bool playerWillDie` snapshot BEFORE the erase (the "cheap" option from CR-2). **[No]**.
- **CR-1 EventBus shared_mutex / lying comment (l.52-79)** — fix comment, add thread-id assertion. **[Partial]** — comment fixed (`EventBus.h:97-107`). Thread-id `assert` NOT added (no `thread::id` in `src/EventBus.cpp`). **[Stale-doc-only]** — recommendation partially honored.
- **CR-2 Player* dangling after erase (l.81-104)** — index/weak_ptr replacement. **[Yes]** — `playerWillDie` bool snapshot at `GameController.cpp:529`; sweep comment cites `[basic.stc.dynamic.deallocation]/4` explicitly. **[No]**.
- **CR-3 stack-captured singleton lifetime (l.106-123)** — self-unsubscribe at end of main(). **[Yes]** — `GameController::~GameController()` calls `EventBus::Instance().Clear()` at `GameController.cpp:110-115`; same effect (drops subscribers before captured refs die). **[No]**.
- **CR-4 WireDefaultSubscribers 3-axis (l.126-142)** — split into 3 functions. **[Yes]** — `include/EventWiring.h:13-18` `WireLoggingSubscribers`, `:26-69` `WireStateTransitionSubscribers`, `:80-83` `WireHudMessageSubscriber`; aggregator `WireDefaultSubscribers` preserved at `:87-94`. **[No]**.
- **CR-5 main() is god function (l.144-161)** — 7-axis, extract `BootstrapWindow`/`SeedDefaultObjects`/etc. **[Partial]** — `src/main.cpp` is now **120 lines** (was 223) but still owns title/select/EventBus wiring/loop/restart flow inline. The MVC composition-root design is intentional (CLAUDE.md §5 "main.cpp is a thin composition root"). **[Stale-doc-only]** — original 223 lines reduced 46%; "7-axis" critique no longer fits but root not exploded into helpers.
- **DIP-only Factory is switch-on-enum (l.169-178)** — refactor to Creator hierarchy or rename claim. **[No]** — `src/GameObjectFactory.cpp:14-32` still a single `switch(type)`. **[Real conflict]** — open recommendation, neither path taken.
- **DIP-only Player::LoadSprite path leak (l.180-186)** — inject asset layer. **[No]** — `include/Player.h:77` still `void LoadSprite(const std::string& path)`. **[Real conflict]** — open recommendation, deliberately deferred (assignment OOP shape).
- **Concurrency-only Publish bad_alloc (l.188-194)** — caller has no exception annotation. **[No]** — `src/EventBus.cpp:38` `void Publish(...) const` (no `noexcept`); SKIP list line 288 explicitly rejects `noexcept`. **[Stale-doc-only]** — finding self-superseded by R3's own SKIP list.
- **Performance-only frameColliders per-frame copy (l.196-201)** — hoist scratch buffer. **[Yes]** — `src/GameController.cpp:101` `frameColliders_.reserve(64)` in ctor; `:400-409` `clear()`+`push_back` per frame; member-scoped scratch buffer. **[No]**.
- **SRP-only Vendor::TryBuy hardcodes UI (l.203-208)** — extract to VendorMessages.h. **[Yes]** — `include/VendorMessages.h:13,16` exists with `kInsufficientFunds`/`kPurchasedPrefix`; `src/Vendor.cpp:4` includes it. **[No]**.
- **STRICT C1 Cursed/ProfTrap idempotency (l.214)** — confirmed fixed. **[Yes]** — also extended to True/Fragile in Cycle 6 L2 (`b26aa9a`, BUGLEDGER L2). **[No]**.
- **STRICT M1 nodiscard sweep — AddMoney asymmetry (l.215-218)** — gap. **[No]** — `include/Player.h:33-37` `Player& AddMoney(int) noexcept` returns fluent ref (NOT `[[nodiscard]]`); `:99` `[[nodiscard]] bool DeductMoney` still asymmetric. **[Real conflict]** — open SPRINT #4 not actioned.
- **STRICT M6 final on leaves (l.219)** — confirmed actioned. **[Yes]** — historical, gated. **[No]**.
- **STRICT M4 dynamic_cast → virtual (l.220-222)** — confirmed actioned. **[Yes]** — `IsVendor()`/`BlocksMovement` pattern landed (BUGLEDGER I5). **[No]**.
- **Disputed: Player 4-way split REJECTED (l.228-241)** — only Animator extraction (S7). **[No — animator]** — `src/Player.cpp:44-55` still owns `animTimer_`/`animStep_`/`lastFacing_`; no `PlayerAnimator` class. **[Real conflict]** — SPRINT #8 not actioned (consistent with student-game scale).
- **Disputed: EventBus SRP RECLASSIFIED (l.243-248)** — DIP issue, not SRP. **[N/A]** — taxonomic argument. **[No]**.
- **Disputed: Factory satisfies Factory Method REJECTED (l.250-256)** — rename claim or refactor. **[No]** — same as DIP-only; switch unchanged. **[Real conflict]**.
- **SPRINT #1 Fix EventBus comment (l.264)** — **[Yes]** `EventBus.h:97-107`. **[No]**.
- **SPRINT #2 Player* → index handle (l.265)** — **[Yes — bool form]** `GameController.cpp:529`. **[No]**.
- **SPRINT #3 Hoist frameColliders (l.266)** — **[Yes]** `GameController.cpp:101` member-scoped. **[No]**.
- **SPRINT #4 AddMoney nodiscard symmetry (l.267)** — **[No]** `Player.h:33`. **[Real conflict]**.
- **SPRINT #5 Split WireDefaultSubscribers (l.268)** — **[Yes]** `EventWiring.h:13-94`. **[No]**.
- **SPRINT #6 Extract Vendor UI to VendorMessages.h (l.269)** — **[Yes]** `include/VendorMessages.h`. **[No]**.
- **SPRINT #7 Self-unsubscribe via Clear() (l.270)** — **[Yes]** `GameController.cpp:110-115`. **[No]**.
- **SPRINT #8 Extract PlayerAnimator (l.271)** — **[No]** `Player.cpp:44-55`. **[Real conflict]**.
- **SPRINT #9 Factory rename or refactor (l.272)** — **[No]** `GameObjectFactory.cpp:14`. **[Real conflict]**.
- **SPRINT #10 Optional mutex behind compile flag (l.273)** — **[No]** `EventBus.h:107` `shared_mutex` unconditional. **[No]** — explicitly tagged "optional".
- **SKIP list (l.282-290)** — 6 anti-recommendations. **[N/A]** — meta. **[No]**.
- **Reviewer-agreement matrix (l.296-308)** — meta cross-citation table. **[N/A]**. **[No]**.

## Summary

- **Total documented elements:** 31
- **Implemented (Yes):** 16 — CR-1 (comment), CR-2, CR-3, CR-4, Perf frameColliders, SRP Vendor strings, STRICT C1/M1-actioned/M6/M4 (4), SPRINT #1/#2/#3/#5/#6/#7
- **Partial:** 2 — CR-1 (thread-id assert missing), CR-5 (main 120 lines, not exploded)
- **Not implemented (No):** 7 — DIP Factory, DIP LoadSprite, Conc bad_alloc (self-skipped), AddMoney nodiscard asymmetry (SPRINT #4), PlayerAnimator (SPRINT #8), Factory rename/refactor (SPRINT #9), Perf mutex flag (SPRINT #10, optional)
- **N/A / meta:** 6 — TL;DR roll-ups, reviewer matrix, Disputed EventBus taxonomy, SKIP list, banner, Cursed-idempotency note
- **Conflicts found:** **5 Real conflicts** (DIP Factory switch, DIP Player::LoadSprite path, AddMoney/DeductMoney nodiscard asymmetry, PlayerAnimator un-extracted, Factory not renamed/refactored) — all open recommendations, all consistent with CLAUDE.md §3 (assignment OOP shape preserved) / Cycle-5 "by-design" stance; **3 Stale-doc-only** (banner self-supersedes C-1; Conc bad_alloc self-skipped; CR-5 line count outdated). No regressions; no R3 finding describes current behavior worse than the doc claims.
