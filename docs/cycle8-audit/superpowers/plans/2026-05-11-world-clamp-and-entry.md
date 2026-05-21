# Audit — docs/superpowers/plans/2026-05-11-world-clamp-and-entry.md

**Overview (≤3):**
1. All eight planned tasks are implemented in shipped code: `gfx::ClampToWorld`, `Camera2D::ClampToWorld`, `WorldConfig`, `Character::SetPosition`, `EventType::EnteredBuilding`, `Buildings.h` registry, `BuildingTracker`, plus full wiring. Wiring moved from `main.cpp` to `GameController.cpp` / `View.cpp` / `EventWiring.h` to honor the MVC red lines added after the plan was written — this is a real (and beneficial) architectural divergence, not stale doc.
2. `Buildings.h` evolved beyond the plan: count 27 → 26 (羅馬廣場 baked into the base map), schema gained `flipX/flipY` mirror flags, coordinates regenerated from Tiled (`tools/tiled_to_world.py`), not transcribed from `composite_worldmap.py` as planned. The plan's hard-coded test coordinates (操場 at 900,360; 體育館 at 1450,360) no longer match shipped rects — would-be regressions live in `tests/test_building_tracker.cpp`.
3. `BuildingTracker` is stricter than the plan: it now resolves overlapping trigger rects deterministically via `detail::NearestContaining` (nearest centre, lex-tie on name) instead of first-match break. `EventType` enum order changed (no `RenderRequested`, `PickupAcquired` added at the tail) — only naming/order drift, no behavioural conflict.

## Per-element annotations

- **Goal — clamp camera+player to 2048×2048 + 27-building registry + EnteredBuilding event**
  - [是否實作?] Partial — `include/WorldConfig.h:8`, `include/Buildings.h:26` (26, not 27), `include/EventBus.h:14`.
  - [邏輯衝突?] Stale-doc-only — registry shrank to 26 (羅馬廣場 dropped) per `include/Buildings.h:24-25`.

- **Architecture — `Camera2D::ClampToWorld`, free `gfx::ClampToWorld`, state-tracker class, constexpr array, enum value, `SetPosition` on `Character`**
  - [是否實作?] Yes — `include/gfx/Camera2D.h:27`, `include/gfx/Bounds.h:10`, `include/BuildingTracker.h:44`, `include/Buildings.h:26`, `include/EventBus.h:14`, `include/Character.h:26`.
  - [邏輯衝突?] No.

- **Project Invariant 1 — `raylib.h` only inside `include/gfx/*.h`**
  - [是否實作?] Yes — verified by `grep -rn 'raylib\.h' src/ tests/ include/ | grep -v 'include/gfx/'` returning nothing.
  - [邏輯衝突?] No.

- **Project Invariant 4 — `objects.erase` after `DrawScope`**
  - [是否實作?] Yes — but now in `GameController.cpp` end-of-frame, not `main.cpp` (CLAUDE.md §5 MVC red line). `main.cpp:91` is now thin composition.
  - [邏輯衝突?] Stale-doc-only — main.cpp no longer hosts the loop body.

- **File `include/WorldConfig.h` (Task 3)**
  - [是否實作?] Yes — `include/WorldConfig.h:1-17` (`kSize=2048`, `kPlayerWidth=24`, `kPlayerHeight=24`).
  - [邏輯衝突?] No.

- **File `include/gfx/Bounds.h` — `nccu::gfx::ClampToWorld(pos,size,worldSize)` (Task 1)**
  - [是否實作?] Yes — `include/gfx/Bounds.h:10-21`.
  - [邏輯衝突?] No (semantics match plan: AABB-in-world, oversize → pin 0).

- **File `include/Buildings.h` — `constexpr array<Building,27>` (Task 6)**
  - [是否實作?] Partial — `include/Buildings.h:26` ships `std::array<Building,26>`; struct gained `flipX/flipY` (`Buildings.h:15-16`). Coords regenerated from Tiled, not `composite_worldmap.py`.
  - [邏輯衝突?] Real — count 27→26 and coordinate set drift. Plan-listed coords (操場 720,180,360,360) no longer match shipped (操場 1384,541,621,399). Plan is stale.

- **File `include/BuildingTracker.h` + `src/BuildingTracker.cpp` (Task 7)**
  - [是否實作?] Yes — `include/BuildingTracker.h:44`, `src/BuildingTracker.cpp:9`.
  - [邏輯衝突?] Stale-doc-only — implementation now resolves overlaps via `detail::NearestContaining` (`BuildingTracker.h:18-36`), not first-match break. Event payload reduced to `{type,text}` (plan referenced `Event` carrying position+color, but shipped `Event` is type+text only — `EventBus.h:21-24`).

- **Test files `test_bounds.cpp`, `test_camera2d_clamp.cpp`, `test_building_tracker.cpp`**
  - [是否實作?] Yes — present under `tests/`.
  - [邏輯衝突?] N/A (tests track shipped data, not the plan's stale coords).

- **Modify `include/gfx/Camera2D.h` — add `ClampToWorld(worldSize, viewportSize)` (Task 2)**
  - [是否實作?] Yes — `include/gfx/Camera2D.h:27-37`. Fluent `Camera2D&` return preserved.
  - [邏輯衝突?] No.

- **Modify `include/Character.h` — add `SetPosition(Vec2)` (Task 4)**
  - [是否實作?] Yes — `include/Character.h:26-30` with hit-box sync.
  - [邏輯衝突?] No.

- **Modify `include/EventBus.h` — add `EventType::EnteredBuilding` after `ShowMessage` (Task 5)**
  - [是否實作?] Yes — `include/EventBus.h:14`. Enum order is `UmbrellaClaimed, KarmaChanged, ShowMessage, EnteredBuilding, PickupAcquired`.
  - [邏輯衝突?] Stale-doc-only — plan listed `RenderRequested` first (removed; render goes through `View`, not events). `PickupAcquired` added post-plan.

- **Modify `src/main.cpp` — wire clamp + tracker + HUD + EnteredBuilding subscriber (Task 8)**
  - [是否實作?] Partial — wiring exists but moved out of `main.cpp` (now thin per CLAUDE.md §5).
    - Player clamp: `src/GameController.cpp:390-393`.
    - Camera clamp: `src/View.cpp:71` chained after `Follow`.
    - `BuildingTracker.Update`: `src/GameController.cpp:494`.
    - `currentBuildingName.clear()` on null: `src/GameController.cpp:495`.
    - `EnteredBuilding` subscriber: `include/EventWiring.h:31-35`.
    - HUD label render: `src/View.cpp` (uses `World::CurrentBuildingName()`).
  - [邏輯衝突?] Real (intentional) — wiring is split across MVC layers, not inlined in `main.cpp`. `World` now owns `tracker_` and `currentBuildingName_` (`World.h:138,140`); `GameController` calls `world_.Tracker().Update(...)`. Better architecture than the plan's god-main; plan is stale.

- **Rationale: player clamp before E-key interaction**
  - [是否實作?] Yes — `GameController.cpp:390` precedes `Key::E` block at `GameController.cpp:415`.
  - [邏輯衝突?] No.

- **Rationale: tracker uses player CENTRE not corner**
  - [是否實作?] Yes — `GameController.cpp:490-493` adds half playerSize.
  - [邏輯衝突?] No.

- **Rationale: HUD prefix "Inside: " ASCII fallback for CJK**
  - [是否實作?] No — `nccu::gfx::EnsureFont()` in `main.cpp:40` loads a CJK atlas; HUD renders native CJK building names. Plan's "fallback boxes" caveat is obsolete.
  - [邏輯衝突?] Stale-doc-only.

- **Task 9 verification gate (clean build, 49 doctests, smoke survives, red-line greps)**
  - [是否實作?] N/A — verification artefacts not in repo; CHANGELOG/BUGLEDGER reflects post-merge integration (Cycle 8 work is downstream of Phase B+C).
  - [邏輯衝突?] N/A.

- **Self-Review §4.1-§4.8 spec coverage map**
  - [是否實作?] Yes — every listed sub-element implemented (with the divergences above).
  - [邏輯衝突?] No.

- **Intentional simplifications: rectangle-AABB clamp (no walkable mask), no LeftBuilding event, ASCII HUD prefix**
  - [是否實作?] Superseded — pixel-accurate `CollisionMask` shipped (`include/CollisionMask.h:20`), loaded via `World::terrainMask_` (`World.cpp:55`) and consumed by `nccu::physics::ResolveMove` (`GameController.cpp:407`). LeftBuilding still absent; HUD line still clears via `clear()` on null (`GameController.cpp:495`). CJK font shipped, ASCII prefix dropped.
  - [邏輯衝突?] Stale-doc-only — all three simplifications either resolved or kept by design.

## Summary

- Total elements audited: **20**
- Yes (fully implemented as planned): **10**
- Partial: **4** (Goal/27→26; Buildings count+schema; main.cpp wiring split; superseded simplifications)
- No: **1** (ASCII "Inside: " HUD prefix — replaced by CJK font)
- N/A: **2** (Task 9 verification artefacts; tests vs stale plan coords)
- Stale-doc-only divergences: **8**
- Real conflicts: **2** (Buildings.h count+coords; main.cpp wiring relocated to MVC layers)
- Both real conflicts are beneficial deltas (Tiled-driven authoring + MVC purity), not regressions.
