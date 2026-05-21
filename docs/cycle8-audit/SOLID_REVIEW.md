# Audit — docs/SOLID_REVIEW.md

**Overview (≤3):**
1. The file already self-declares "SUPERSEDED in part (Cycle 3 verify)" at the top, and BUGLEDGER Cycle-3 footer confirms `#2/#3 reference deleted code` (`.claude/BUGLEDGER.md:605`). Roughly two thirds of the immediate-action items have shipped; the rest are explicit `TIGHTEN_LATER` deferrals or `REJECT_AS_OVERENGINEERED` non-decisions, all of which still match current code.
2. The lone REAL_VIOLATION ("ProfessorTrapUmbrella idempotency guard") is implemented (`src/ProfessorTrapUmbrella.cpp:7`); the suggested `unordered_map` building→state mapping was superseded by `WireStateTransitionSubscribers` in `include/EventWiring.h:31` (different mechanism but resolves the same SRP smell); the `dynamic_cast` removal landed via virtual predicates in `include/GameObject.h:35,42,49,57`.
3. No real conflicts: the "deleted code" notes in items #2/#3 are Stale-doc-only (the file flags them itself). Deferred / rejected items are still consistent with HEAD `b33db2b` (Vendor still 93 LOC, Player still 159 LOC, Factory still enum+switch, no IWallet/IPlayerState, EventBus still singleton).

## Per-element annotations

- **Header SUPERSEDED banner** — `main.cpp` no longer has the building-string map or the `dynamic_cast<NPC*>` at line 210.
  - [是否實作?] N/A — self-annotation; verified `src/main.cpp` is now 120 LOC composition root (banner says 67; banner slightly stale on line count but accurate on substance).
  - [邏輯衝突?] No — Stale-doc-only (line count drift).

- **整體結論 S row — defer Player split, find EventBus.Publish policy + Umbrella 3-axis** — keep god-class-fear-mongering at bay.
  - [是否實作?] Yes — Player still single-class (`src/Player.cpp` 159 LOC); EventBus.Publish snapshot policy documented in `src/EventBus.cpp:38-55` comment; Umbrella 3-axis stayed merged.
  - [邏輯衝突?] No.

- **整體結論 O row — keep enum+switch; main building-string mapping → unordered_map; dynamic_cast 兩處下次再修** — OCP pragmatism.
  - [是否實作?] Partial — enum+switch preserved (`src/GameObjectFactory.cpp:14-31`); building-string mapping was instead solved by event-driven wiring in `include/EventWiring.h:31` (superseded mechanism); both `dynamic_cast`s gone from main.cpp (one survives in `src/World.cpp:42` Player-anchor lookup, intentional).
  - [邏輯衝突?] No — Stale-doc-only (mechanism differs from suggestion but achieves the same outcome).

- **整體結論 L row — ProfessorTrapUmbrella idempotency** — single REAL_VIOLATION.
  - [是否實作?] Yes — `src/ProfessorTrapUmbrella.cpp:7` `if (spawnedEnemiesCount_ > 0) return;`. Family-wide pattern (`Fragile/True/Cursed` also guard at line 7).
  - [邏輯衝突?] No.

- **整體結論 I row — leave fat GameObject base; record IUpdatable/IDrawable/IInteractable as next-iter direction** — ISP deferral.
  - [是否實作?] N/A (deferred). `include/GameObject.h:19-21` retains the three pure virtuals; no role interfaces shipped.
  - [邏輯衝突?] No.

- **整體結論 D row — keep EventBus singleton; only fix main string mapping** — DIP pragmatism.
  - [是否實作?] Yes — `EventBus::Instance()` static singleton at `src/EventBus.cpp:7`; no `IEventSink`.
  - [邏輯衝突?] No.

- **#1 ProfessorTrapUmbrella idempotency guard** — REAL_VIOLATION fix.
  - [是否實作?] Yes — `src/ProfessorTrapUmbrella.cpp:7`; `spawnedEnemiesCount_>0` guard exactly as specified, then `isActive_=false`, then publishes.
  - [邏輯衝突?] No.

- **#2 main.cpp 樓宇 → SemesterState unordered_map** — TIGHTEN_NOW SRP fix.
  - [是否實作?] Yes (superseded) — banner already flags. Mechanism shipped as `WireStateTransitionSubscribers` lambdas in `include/EventWiring.h:31` + `BuildingTracker` publishing `EnteredBuilding` (`src/BuildingTracker.cpp:16`). No `kEnterTrigger` map but the policy left `main.cpp`.
  - [邏輯衝突?] No — Stale-doc-only.

- **#3 main.cpp two `dynamic_cast`s → virtual predicate** — TIGHTEN_NOW OCP fix.
  - [是否實作?] Yes — `include/GameObject.h:35` `BlocksMovement()`, `:42` `DialogLines()`, `:49` `NpcId()`, `:57` `IsVendor()`; NPC override at `include/NPC.h:34`. Main no longer dynamic_casts; only `src/World.cpp:42` remains (player anchor — different purpose).
  - [邏輯衝突?] No.

- **#4 GameObjectFactory enum+switch → registry (TIGHTEN_LATER)** — explicit deferral.
  - [是否實作?] No (intentional defer) — `src/GameObjectFactory.cpp:14-32` is the documented `switch (type)` block; no static-init registry.
  - [邏輯衝突?] No.

- **#5 ISP refactor: lifecycle-only base + role interfaces (TIGHTEN_LATER)** — deferred design sketch.
  - [是否實作?] No (intentional defer); no `IUpdatable`/`IDrawable`/`IInteractable` in `include/`.
  - [邏輯衝突?] No.

- **#5 follow-up — awsome_cpp.md §4/§17 trade-off note + UML.md tradeoff** — companion doc work.
  - [是否實作?] Partial — `awsome_cpp.md` and `UML.md` both exist at repo root; not verified whether the specific "abstract vs interface" subsection landed (out of read-only audit scope).
  - [邏輯衝突?] No.

- **#6 Vendor IWallet / Item IPlayerState (TIGHTEN_LATER)** — DIP deferral.
  - [是否實作?] No (intentional defer) — `Player::DeductMoney` (`src/Player.cpp:111`) and `Player::AddMoney` (`include/Player.h:33`) remain concrete; no `IWallet` interface.
  - [邏輯衝突?] No.

- **#7 REJECT — Player split into 4 classes** — over-engineering reject.
  - [是否實作?] No (intentional reject). Player.cpp = 159 LOC, well under the 500 LOC reconsideration threshold.
  - [邏輯衝突?] No.

- **#8 REJECT — Vendor split into builder/policy/publisher** — over-engineering reject.
  - [是否實作?] No (intentional reject). Vendor.cpp = 93 LOC.
  - [邏輯衝突?] No.

- **#9 REJECT — EventBus → IEventSink injection (with SetTestInstance fallback)** — DIP reject.
  - [是否實作?] No (intentional reject). No `IEventSink`, no `SetTestInstance`. Test isolation handled by global `Clear()` via doctest listener (`tests/test_eventbus_isolation.cpp` per BUGLEDGER B2) plus H1's `ScopedSubscribe` (`src/EventBus.cpp:18-24`).
  - [邏輯衝突?] No.

- **新發現 #1 — EventBus::Publish snapshot policy hidden in bus** — SRP escape valve.
  - [是否實作?] Yes — `src/EventBus.cpp:38-55` carries a 9-line comment explicitly documenting the snapshot rationale (reentrancy + writer-stability). Action item ("加註釋") satisfied; BUGLEDGER H1 explains the RAII Subscription rules.
  - [邏輯衝突?] No.

- **新發現 #2 — TransparentUmbrella(/ProfessorTrap) 3-axis merge: Draw + Interact + OnPickup all → beClaimed** — known trade-off, no refactor.
  - [是否實作?] N/A (intentional log-only). All `*Umbrella::beClaimed` paths still merged; UML.md exists at repo root (specific trade-off subsection not verified — read-only).
  - [邏輯衝突?] No.

- **新發現 #3 — Character leaks speed_/direction_/currentFrame_ to NPC** — ISP debt.
  - [是否實作?] No (intentional defer along with #5).
  - [邏輯衝突?] No.

- **哲學總結 + 引述** — methodology recap.
  - [是否實作?] N/A — non-actionable prose.
  - [邏輯衝突?] No.

- **方法論 section** — two-stage adversarial review process note.
  - [是否實作?] N/A — methodology metadata.
  - [邏輯衝突?] No.

## Summary

- Total elements: 19
- Implemented (Yes): 7 (#1 idempotency, #3 virtual predicates, S/L/D rows, new-finding #1, item #2 outcome via superseded mechanism)
- Partial: 2 (item #2 mechanism-superseded; #5 follow-up docs)
- Intentional deferrals / rejects (No): 7 (#4, #5, #6, #7, #8, #9, new-finding #3)
- N/A (methodology / banner / log-only): 3
- Logical conflicts: 0 Real conflicts, 2 Stale-doc-only (header SUPERSEDED banner already self-flagged; item #2 shipped as event wiring rather than the literal `unordered_map`)
- Net status: file is a faithful design baseline with explicit "SUPERSEDED" annotation; no Real conflicts vs HEAD `b33db2b`.
