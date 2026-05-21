# Audit — docs/superpowers/specs/2026-05-11-world-clamp-and-entry-design.md

**Overview (≤3):**
- The Phase B + C feature shipped in spirit: world AABB clamp (camera + player), `Buildings.h` registry, `BuildingTracker` edge detector, `EnteredBuilding` event, and `Inside: …` HUD line are all live and exercised by tests + harness JSONL.
- Numeric/structural deviations vs spec: registry is **26 buildings, not 27** (羅馬廣場 intentionally omitted, per `Buildings.h` comment) and tracker uses **nearest-centre disambiguation**, not first-match-by-array-order. Spawn coords also changed (`Vec2{500,1860}` instead of the spec's "Phase A `{400,225}` stays").
- Wiring rehomed: clamp + tracker live in `GameController::Update` and `View::Render`, not `main.cpp`. The current-building string is **owned by `World` (`currentBuildingName_`) and pushed by `WireStateTransitionSubscribers`** in `EventWiring.h`, not by a `main.cpp` subscriber. Stale-doc, not a real conflict.

## Per-element annotations

- **§4.1 `WorldConfig.h` (kSize=2048, kPlayerWidth/Height=24)** — central world/player magic numbers.
  - [是否實作?] Yes — `include/WorldConfig.h:8,11-12`
  - [邏輯衝突?] No

- **§4.2 `Camera2D::ClampToWorld(worldSize, viewportSize)`** — fluent target clamp, midpoint pin when world < viewport.
  - [是否實作?] Yes — `include/gfx/Camera2D.h:27-37`
  - [邏輯衝突?] No

- **§4.3 free helper `gfx::ClampToWorld(pos,size,worldSize)`** — pure rectangle clamp, pinning to 0 when size > world.
  - [是否實作?] Yes — `include/gfx/Bounds.h:10-21`
  - [邏輯衝突?] No

- **§4.4 `Buildings.h` registry of "27" with `triggerRect = {cx-h/2, cy-h/2, h, h}` square trigger zones, Python-order**.
  - [是否實作?] Partial — `include/Buildings.h:26-53`. Registry exists but is **26 entries** (羅馬廣場 deliberately excluded), rects are **non-square** ({x,y,w,h} from Tiled tool, e.g. `{1776,1021,252,211}`), order matches `tools/tiled_to_world.py` not `composite_worldmap.py`, and a `flipX/flipY` pair was added to `Building`.
  - [邏輯衝突?] Yes — Stale-doc-only. Tiled became the upstream of placements (CHANGELOG: "Auto-emitted by tools/tiled_to_world.py"); spec's `composite_worldmap.py` ingredients and square-trigger convention are superseded.

- **§4.5 `BuildingTracker` single-state edge detector with point-in-rect lookup**.
  - [是否實作?] Yes — `include/BuildingTracker.h:44-52`, `src/BuildingTracker.cpp:9-20`
  - [邏輯衝突?] Yes — Stale-doc-only. Implementation uses `detail::NearestContaining` (nearest-centre + lexical tie-break) rather than first-match, explicitly because Tiled-emitted rects overlap. Spec §10 promised "buildings are non-overlapping by registry construction" — that invariant no longer holds; the tracker correctly handles it.

- **§4.6 `EventType::EnteredBuilding` added; reuses `Event::text`**.
  - [是否實作?] Yes — `include/EventBus.h:14`; payload carries `text` only.
  - [邏輯衝突?] No (spec said `position` also reused; not needed in practice — no real conflict).

- **§4.7 `Character::SetPosition(Vec2)` mutator (placed on `Character`, not `Player`)**.
  - [是否實作?] Yes — `include/Character.h:26-30`
  - [邏輯衝突?] No

- **§4.8 `main.cpp` wiring (clamp after Update, tracker, camera Follow+Clamp, HUD line at (10,50), stdout subscriber)**.
  - [是否實作?] Partial — sequence is preserved but rehomed: player clamp at `src/GameController.cpp:390` post-Update; tracker `world_.Tracker().Update(playerCentre)` at `src/GameController.cpp:491-496`; camera clamp at `src/View.cpp:71` (`Follow(...).ClampToWorld(...)`); HUD `Inside: …` line in panel at `src/View.cpp:138-140,195`; subscriber `[Game] Entered: <name>` at `include/EventWiring.h:31-35` (also stored to `currentBuildingName_`).
  - [邏輯衝突?] Yes — Stale-doc-only. Spec located all of this in `main.cpp`; MVC refactor moved the input/sim half to `GameController` and the view half to `View`. HUD line is now a translucent panel row, not a pinned (10,50) text. No behavioural conflict.

- **§5 Pattern map (Fluent setter, Pure function, Static data table, 1-state edge detector, Observer)** — taxonomy of the above.
  - [是否實作?] Yes — matches code.
  - [邏輯衝突?] No

- **§6 Touch points (`Camera2D.h`, `Character.h`, `EventBus.h`, `main.cpp`, no CMake change) + new files (`WorldConfig.h`, `gfx/Bounds.h`, `Buildings.h`, `BuildingTracker.h/.cpp`)**.
  - [是否實作?] Yes — every new file exists; touch-point edits landed.
  - [邏輯衝突?] No (real wiring is in `GameController.cpp`/`View.cpp`, not `main.cpp` — stale-doc only).

- **§7 Tests (`test_camera2d_clamp.cpp`, `test_bounds.cpp`, `test_building_tracker.cpp`)**.
  - [是否實作?] Yes — `tests/test_camera2d_clamp.cpp`, `tests/test_bounds.cpp`, `tests/test_building_tracker.cpp` all present; tracker test subscribes to `EnteredBuilding` and uses `CentreOf(name)` lookups in `kAll`.
  - [邏輯衝突?] No

- **§8 Verification gate (warnings, ctest, ≥5 s smoke, manual walks, raylib confinement, naming hygiene, UTF-8)**.
  - [是否實作?] N/A — gate procedure, not code.
  - [邏輯衝突?] No

- **§9 Implementation order suggestion** — pure procedural guidance.
  - [是否實作?] N/A
  - [邏輯衝突?] No

- **§10 Risks/invariants: UTF-8, coord convention, player-size link, EventBus snapshot, non-overlap, HUD layout at (10,50)**.
  - [是否實作?] Partial — UTF-8/snapshot/player-size invariants hold; HUD moved into the translucent panel (`src/View.cpp:138-195`).
  - [邏輯衝突?] Yes — Real conflict on the "non-overlapping by construction" invariant: Tiled-emitted rects DO overlap, mitigated in code by `NearestContaining` rather than by registry layout. Stale-doc on "HUD at (10,50)".

- **§11 Out-of-scope: `LeftBuilding`, interiors, highlight, CJK TTF, walkable mask, smooth camera**.
  - [是否實作?] N/A for the deferred items; **walkable mask was actually built** later — `include/CollisionMask.h:20-78` + `src/World.cpp:55` (`terrainMask_ = LoadTerrainMask();`) + Phase B2 resolver call at `src/GameController.cpp:393-410`.
  - [邏輯衝突?] Yes — Stale-doc-only. Spec parked terrain collision; subsequent phase shipped it.

- **Spec §3 non-goal: "Phase A `Vec2{400,225}` start stays"** — player spawn unchanged.
  - [是否實作?] No — spawn is `Vec2{500, 1860}` at `src/World.cpp:28` (and respawn at `src/Player.cpp:154`).
  - [邏輯衝突?] Yes — Stale-doc-only. Map/coords were rebuilt for 山下 layout; non-goal voided.

## Summary

- Elements audited: 16
- Yes: 9 · Partial: 3 · No: 1 · N/A: 3
- Conflicts: 6 total — 5 stale-doc-only (registry size/source, tracker disambiguation, main.cpp wiring location, HUD coords, walkable-mask now built, spawn coord), 1 real-but-mitigated (overlapping triggers vs. spec's non-overlap claim, handled by `NearestContaining`).
