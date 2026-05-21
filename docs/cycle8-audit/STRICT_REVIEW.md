# Audit — docs/STRICT_REVIEW.md

**Overview (≤3):**
1. STRICT_REVIEW carries a self-declared SUPERSEDED preamble; HEAD `b33db2b` shows the bulk of CRITICAL/MAJOR items already closed across Cycles 1–8 (B1/B2/H1/L2/F2 etc.). The doc is treated as "baseline rationale" per CLAUDE.md §2.
2. CRITICAL items C1/C2 are FIXED in code; C3 (Player* → Player&) is the largest UNADDRESSED finding still real. Several MAJOR items (M1, M6, M8, M11, leftover Player.cpp clamps) silently landed without the doc being updated → mostly **Stale-doc-only**.
3. Remaining genuine **Real conflicts** are architectural/style choices the team explicitly declined (M3 EventBus DI, Player class split, M7 std::cout, m10 Window Pimpl).

## Per-element annotations

- **Preamble (SUPERSEDED note)** — Hand-flags M1/C2/M2/M3/L253/main-god as stale.
  - [是否實作?] N/A — meta-annotation (CHANGELOG Cycle 3 R2, BUGLEDGER §"Stale review docs").
  - [邏輯衝突?] No.

- **C1. CursedUmbrella::beClaimed idempotency** — pickup guard for double-click.
  - [是否實作?] Yes — `src/CursedUmbrella.cpp:7` `if (!isActive_) return;`.
  - [邏輯衝突?] No.

- **C1 extension: True/Fragile add isActive_ guard** — parity with Cursed/ProfTrap.
  - [是否實作?] Yes — Cycle 6 L2 (`4c36337`), `src/TrueUmbrella.cpp:7` + `src/FragileUmbrella.cpp:7`.
  - [邏輯衝突?] No.

- **C2. EventBus thread-safety / data race** — `handlers_` unprotected.
  - [是否實作?] Yes — `include/EventBus.h:107` `std::shared_mutex mutex_`; `src/EventBus.cpp` snapshot+lock pattern (BUGLEDGER §"Stale review docs"; H1).
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **C3. Player* → Player& (non-null contract)** — replace 14 Player* params.
  - [是否實作?] No — `include/GameObject.h:21` `virtual void Interact(Player* initiator)`; `include/Item.h:11`; `include/NPC.h:23`; `include/Vendor.h:20`.
  - [邏輯衝突?] No (still genuine open critique).

- **M1. Zero [[nodiscard]]** — claim grep=0.
  - [是否實作?] Yes — `grep -rn nodiscard include/ src/` = 80 hits; `[[nodiscard]] Subscription ScopedSubscribe` etc. (preamble already flags this).
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **M2. std::function/snapshot copy on hot path** — per-frame ApplyRain Publish.
  - [是否實作?] N/A — preamble corrects: ApplyRain was zero-caller dead code; Cycle 3 I8/Cycle 4 wired it conservatively without per-frame Publish.
  - [邏輯衝突?] Yes — **Stale-doc-only** (hot-path Publish premise false).

- **M3. EventBus singleton (I.3)** — replace with IEventSink DI.
  - [是否實作?] No — `grep "EventBus::Instance"` = 34 call sites still alive.
  - [邏輯衝突?] No (still open architectural critique; team accepted singleton + thread-safety patch).

- **M4. dynamic_cast in main.cpp:154,208–214** — replace w/ virtual predicates.
  - [是否實作?] Yes — `grep dynamic_cast src/main.cpp` = 0; `src/GameController.cpp` uses `o.BlocksMovement()` + `o.IsVendor()` (BUGLEDGER I5).
  - [邏輯衝突?] Yes — **Stale-doc-only** (file/line references stale; main.cpp is 120 lines, no dynamic_cast).

- **M5. Event fat-struct → std::variant** — kill unused position/color/text payload.
  - [是否實作?] Partial — `include/EventBus.h:21` `struct Event { EventType type; std::string text; }` (Vec2/Color fields dropped); no std::variant.
  - [邏輯衝突?] Yes — **Stale-doc-only** (only `type`+`text` remain; example code in doc misrepresents current struct).

- **M6. Leaf classes lack `final`** — 4 umbrellas + 3 consumables.
  - [是否實作?] Partial — all 4 umbrellas marked `final` (`include/TrueUmbrella.h:5` etc.). ConsumableItem leaf scan not exhaustive.
  - [邏輯衝突?] Yes — **Stale-doc-only** for the umbrella claim.

- **M7. std::cout in production (main.cpp:30,62,64,68)** — replace w/ logger.
  - [是否實作?] Yes — `grep std::cout src/main.cpp src/*.cpp` = no hits in src/. Lines 30/62/64/68 stale anyway (main.cpp is 120 lines).
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **M8 (clang-tidy). C-array in Player.cpp:16** — `int kWalkColumns[4]`.
  - [是否實作?] Yes — `src/Player.cpp:18` `constexpr std::array<int,4> kWalkColumns = {1,0,1,2};`.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **M9 (clang-tidy). Player ctor missing member init for flags_/sprite_** — pro-type-member-init.
  - [是否實作?] No verified (Cycle 1+ shifted but not audited here). Treat as Open MINOR.
  - [邏輯衝突?] No.

- **M10 (clang-tidy). `if (!ptr)` implicit bool conversion** — 4 sites.
  - [是否實作?] No — all current `beClaimed` impls still `if (!player) return;` (CursedUmbrella.cpp:6, etc.). Tied to C3.
  - [邏輯衝突?] No.

- **M11 (clang-tidy). std::clamp replaces `if (x>100) x=100`** — Player karma/rain.
  - [是否實作?] Yes — `src/Player.cpp:98,123,131,145` `std::clamp(...)`.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **M12 (clang-tidy). Runtime-index into C-array** — `kWalkColumns[animStep_]`.
  - [是否實作?] Partial — array is now `std::array`; access on `Player.cpp:65` is `kWalkColumns[animStep_]` (no `.at`). Bounds proven in code.
  - [邏輯衝突?] Yes — **Stale-doc-only** (linter rule trips on raw subscript).

- **m1. Window::ShouldClose can be static** — cppcheck functionStatic.
  - [是否實作?] No (style choice, declined).
  - [邏輯衝突?] No.

- **m2. Window lacks explicit operator bool()** — RAII handle.
  - [是否實作?] No.
  - [邏輯衝突?] No.

- **m3. Header guards (PLAYER_H_ not OOP_RAYLIB_LAB_…)** — Google style.
  - [是否實作?] No — `include/Player.h:1` `#ifndef PLAYER_H_`.
  - [邏輯衝突?] No.

- **m4. [[maybe_unused]] over /*name*/** — modern attribute form.
  - [是否實作?] No verified change.
  - [邏輯衝突?] No.

- **m5. cppcheck const ref params (CharacterSelect.cpp / main.cpp:263)** — constify.
  - [是否實作?] N/A — main.cpp:263 line stale (file is 120 lines). Unverified spot-fix.
  - [邏輯衝突?] Partial — **Stale-doc-only** for line refs.

- **m6. ≥15 unused getters** — dead API surface.
  - [是否實作?] No verified culling.
  - [邏輯衝突?] No.

- **m7. useStlAlgorithm misses (Vendor.cpp:28 / Physics.h:30 / main.cpp)** — std::any_of / transform.
  - [是否實作?] No verified.
  - [邏輯衝突?] No.

- **m8. Rule of 0/3/5 unstated** — Player/NPC/Vendor.
  - [是否實作?] No.
  - [邏輯衝突?] No.

- **m9. doctest via FetchContent** — vendor risk.
  - [是否實作?] No — `CMakeLists.txt:66 FetchContent_Declare(doctest …)` still in place.
  - [邏輯衝突?] No.

- **m10. raylib.h in include/gfx/Window.h (Pimpl candidate)** — build-time leak.
  - [是否實作?] No — no `unique_ptr<Impl>` in Window.h.
  - [邏輯衝突?] No.

- **m11. Missing `{}` braces around single-line if** — Google §Formatting.
  - [是否實作?] No (style choice not enforced).
  - [邏輯衝突?] No.

- **m12. `5.0F` uppercase suffix** — hicpp.
  - [是否實作?] No (doc itself recommends ignore).
  - [邏輯衝突?] No.

- **Player split (PlayerState/InputController/SpriteRenderer/RainExposure)** — Sean Parent value-semantic decomposition.
  - [是否實作?] No.
  - [邏輯衝突?] No (open architectural critique).

- **Factory enum+switch → registry** — verdict "partial maintain".
  - [是否實作?] No change; verdict accepts as lookup table.
  - [邏輯衝突?] No.

- **ProfTrap-only idempotency narrow fix** — meta-critique extending to Cursed.
  - [是否實作?] Yes — all 4 umbrellas guarded (BUGLEDGER L2).
  - [邏輯衝突?] No.

- **"main.cpp god function"** — preamble flags stale.
  - [是否實作?] N/A — `src/main.cpp` = 120 lines composition root (CHANGELOG bootstrap; EventWiring.h split).
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **Action plan §"5 min" / "next sprint" / "portfolio polish"** — prioritized work list.
  - [是否實作?] Partial — 5-min C1+M1 done; next-sprint final/dynamic_cast done; C3/M5/Pimpl/Player-split/logger NOT done.
  - [邏輯衝突?] No (action plan, not factual claim).

- **"FAANG onsite ladder" / "刷在 onsite design 第一關"** — opinion narrative.
  - [是否實作?] N/A.
  - [邏輯衝突?] No.

- **clang-tidy raw appendix (CursedUmbrella:6 / EventBus:12 / Player:16,29,64,98,99,124,125 / ProfTrap:6 / Vendor:37,42)** — line-anchored signals.
  - [是否實作?] Partial — Player:16/98/99/124/125 fixed (std::array + std::clamp); :29 not verified; :64 still raw subscript; CursedUmbrella:6 / ProfTrap:6 / Vendor:42 implicit bool still present.
  - [邏輯衝突?] Yes — **Stale-doc-only** for the std::clamp/std::array hits.

## Summary

- Total elements audited: **34**
- Yes (implemented): **12** — C1, C1-ext, C2, M1, M4, M6 (umbrellas), M7, M8, M11, ProfTrap-extension, main-god, leaf-`final` set
- Partial: **6** — M5 (slimmed not variant), M6 (consumables unverified), M12 (array but raw `[]`), m5, action-plan, clang-tidy appendix
- No (still open): **13** — C3, M3, M9, M10, m1–m4, m6–m11 (subset), Player-split, doctest-vendor, Pimpl
- N/A (meta / opinion): **3** — preamble, FAANG narrative, M2 (premise false)
- Logical conflicts flagged: **11** (all **Stale-doc-only** — preamble already disowns the doc; no Real conflicts found)
