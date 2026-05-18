# Phase B + C — World Bounds Clamp & Building Entry Triggers

**Date:** 2026-05-11
**Project:** 《尋傘記：政大山下篇》(`/Users/ian/Desktop/assignment-5-jiangjiangian`)
**Status:** Approved — ready for planning
**Parent design:** `2026-05-11-camera-worldmap-design.md` (Phase A — Camera + worldmap, merged as `22e50bf` on main)

---

## 1. Context

Phase A shipped: the player roams a 2048×2048 worldmap with the camera following them. Two unfinished consequences:

- Walking past the worldmap edge reveals `Colors::RayWhite` outside the texture (camera does not clamp, player does not clamp).
- 27 NCCU buildings are visible on the map but the player cannot interact with them — there are no entry events at all.

Phase B + C close both gaps in a single sub-project: world-bounds clamping (camera AND player) plus building entry triggers driven by a registry sourced from `tools/composite_worldmap.py`'s `BUILDINGS` dict. They share the world-coordinate mental model and the same `src/main.cpp` per-frame slot, so they're shipped together.

## 2. Goals

- Clamp `Camera2D::target` so the viewport never extends past worldmap edges.
- Clamp player world position so the hit-box never extends past worldmap edges.
- Materialise a static `Buildings` registry of all 27 NCCU buildings (name + trigger rect) in C++ headers, matching the positions in `tools/composite_worldmap.py`.
- Detect player → building entry as a single-edge transition (publish on enter, not every frame), then publish a `Event::EnteredBuilding` event carrying the building's display name.
- Render the current building's name as a screen-pinned HUD line ("📍 進入：商學院"–style; emoji optional) while the player is inside any trigger.
- Subscribe to the event in `main.cpp` for `std::cout` logging (consistent with existing UmbrellaClaimed/ShowMessage handlers).

## 3. Non-Goals

- Building interiors / dialogue / NPC spawning (chapter-level work; deferred).
- Chapter gating (some buildings locked early, opened by quest progress).
- LeftBuilding event — one-edge only this phase. (Inferable from current-state tracker; revisit if needed.)
- Per-building custom trigger shapes — all triggers are axis-aligned rects this phase.
- Repositioning the player spawn or umbrellas. Phase A's `Vec2{400, 225}` start stays.
- Smooth camera lerp / easing. Camera still snaps to clamped target.
- Walkable mask (terrain collision beyond the world rectangle). Just rectangle clamp.

## 4. Architecture

### 4.1 World constants

Add `include/WorldConfig.h`:

```cpp
namespace world {
inline constexpr float kSize         = 2048.0f;  // square worldmap, matches worldmap.png
inline constexpr float kPlayerWidth  = 24.0f;    // mirrors Player ctor hit-box
inline constexpr float kPlayerHeight = 24.0f;
}
```

Centralises the magic number `2048.0f` so both the camera-clamp call site and the player-clamp call site read from one place. `kPlayerWidth/Height` mirrors the 24 px square in `Player.cpp`.

### 4.2 Camera clamp

Extend `include/gfx/Camera2D.h`:

```cpp
// Clamp .target so a viewport of `viewportSize` centred on .target
// stays inside the [0, worldSize] AABB. If the world is smaller than
// the viewport on an axis, target stays centred on the world midpoint.
Camera2D& ClampToWorld(Vec2 worldSize, Vec2 viewportSize) noexcept;
```

Implementation is pure data — no raylib calls. Goes in the existing header (header-only inline).

### 4.3 Player clamp

Add a free helper in `include/gfx/Bounds.h`:

```cpp
namespace nccu::gfx {
// Returns pos clamped so a `size`-px AABB anchored at pos stays inside
// the [0, worldSize] AABB. If size > worldSize on an axis, pos pins at 0.
Vec2 ClampToWorld(Vec2 pos, Vec2 size, Vec2 worldSize) noexcept;
}
```

Pure data, header-only. Called once per frame in `main.cpp` after the Update loop but before camera follow. Mutates `Player` position via a new minimal setter (see §4.6) to keep `position_` and `hitBox_` in sync.

### 4.4 Building registry

Add `include/Buildings.h`:

```cpp
#pragma once
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::buildings {

struct Building {
    std::string_view name;          // UTF-8 Chinese display name (e.g., "操場")
    nccu::gfx::Rect  triggerRect;   // world-space AABB
};

// Centre (cx,cy) + triggerSide (= target_height from composite_worldmap.py)
// produces a centred square trigger zone. Order matches the Python dict.
inline constexpr std::array<Building, 27> kAll = {{
    {"操場",        { 720.0f,  180.0f, 360.0f, 360.0f}},
    {"體育館",      {1320.0f,  230.0f, 260.0f, 260.0f}},
    // … 25 more entries …
}};

} // namespace nccu::buildings
```

Coordinates: `triggerRect = {cx - h/2, cy - h/2, h, h}` where `(cx, cy, h)` are the tuple values from `BUILDINGS` in `tools/composite_worldmap.py`. The rect uses the trigger zone as a square aligned to the building tile's painted height — slightly oversized for narrow buildings, which is fine for an entry zone.

`std::string_view` over `const char*` so range-based loops can `std::cout << b.name` directly. UTF-8 source files compile this without `u8""` prefix.

### 4.5 BuildingTracker

Add `include/BuildingTracker.h`:

```cpp
namespace nccu {

class BuildingTracker {
public:
    // Called once per frame with the player's current world position.
    // Returns: pointer to the building the player is INSIDE this frame
    //          (nullptr if none). If the building changed from last
    //          frame, publishes an EnteredBuilding event on EventBus
    //          carrying the building's name.
    const buildings::Building* Update(gfx::Vec2 playerCenter);

    const buildings::Building* Current() const noexcept { return current_; }

private:
    const buildings::Building* current_{nullptr};
};

} // namespace nccu
```

Single mutable field. Pure point-in-rect test for which building contains `playerCenter`. Transition detection: `current_ != newPtr` → fire the event with `newPtr`'s name (if non-null). Leaving a building without entering another → `current_ = nullptr` and no event this phase.

### 4.6 EventBus addition

Extend the existing `EventType` enum in `include/EventBus.h`:

```cpp
enum class EventType {
    RenderRequested,
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
    EnteredBuilding,   // NEW
};
```

The `Event` struct already has `text` (used for building name) and `position` (re-used for the building's trigger-rect centre). No new fields.

### 4.7 Player position setter

Add a minimal mutator to `include/Character.h`:

```cpp
void SetPosition(nccu::gfx::Vec2 p) noexcept {
    position_ = p;
    hitBox_.x = p.x;
    hitBox_.y = p.y;
}
```

Symmetric with the existing `GetPosition()`. Used by main.cpp to apply the clamp result. Keeps hit-box and position in lock-step (same invariant Character::Move maintains).

### 4.8 main.cpp wiring

Per-frame sequence inserts:

```
for each obj: obj->Update(dt)
                                     ← NEW: clamp player to world AABB
if (E pressed && player) interact
                                     ← NEW: tracker.Update(player center)
cam.Follow(player position, screenCenter)
                                     ← NEW: cam.ClampToWorld(...)
DrawScope:
  Renderer::Clear
  CameraScope:
    Renderer::Texture(worldmap, 0,0)
    obj->Draw for each
  HUD:
    "WASD…" line                 (unchanged)
    karma line                   (unchanged)
                                 ← NEW: current building name (if any)
deferred sweep                   (unchanged)
```

The HUD line for the current building uses `TextBuilder`, screen-pinned to (10, 50) below the karma line, with `Colors::Black`.

The new EventBus subscriber prints `[Game] Entered: <name>` to `std::cout`, mirroring the existing `UmbrellaClaimed` subscriber.

## 5. Components & Pattern Map

| Component | Pattern | Notes |
|---|---|---|
| `Camera2D::ClampToWorld` | **Fluent setter** | Returns `Camera2D&`; chains with `Follow(...)` |
| `gfx::ClampToWorld(pos, size, world)` | **Pure function** | Stateless helper, easy to unit-test |
| `Buildings` registry | **Static data table** | `constexpr std::array` — zero-cost at runtime |
| `BuildingTracker` | **State machine (1-state edge detector)** | `current_` is the only state; transition triggers Observer event |
| `EnteredBuilding` event | **Observer (existing EventBus)** | Re-uses existing pub/sub; no infra change |

## 6. Touch Points (existing files)

| File | Change |
|---|---|
| `include/gfx/Camera2D.h` | Add `ClampToWorld` method (~10 lines) |
| `include/Character.h` | Add `SetPosition` (~5 lines) |
| `include/EventBus.h` | Add `EnteredBuilding` enum value (~1 line) |
| `src/main.cpp` | Construct tracker; clamp player; clamp camera; draw HUD building name; subscribe to EnteredBuilding (~15 lines net) |
| `CMakeLists.txt` | No change |

New files: `include/WorldConfig.h`, `include/gfx/Bounds.h`, `include/Buildings.h`, `include/BuildingTracker.h`, `src/BuildingTracker.cpp`, plus three test files.

## 7. Tests

| Test file | Cases |
|---|---|
| `tests/test_camera2d_clamp.cpp` | (a) typical clamp puts target inside `[viewport/2, world − viewport/2]`. (b) when player at (0,0), target clamps to (viewport/2, viewport/2). (c) when player at (2048,2048), target clamps to (world − viewport/2). (d) when world < viewport on an axis, target pins to world midpoint. (e) fluent: returns `*this`. |
| `tests/test_bounds.cpp` | (a) point inside world → unchanged. (b) point past upper-left → pinned to 0. (c) point past lower-right → pinned to `world − size`. (d) size > world → pos pins to 0. |
| `tests/test_building_tracker.cpp` | (a) Initial state: `Current() == nullptr`. (b) Player inside 操場 trigger → `Current() == &kAll[0]` AND one `EnteredBuilding` event fired with `text=="操場"`. (c) Player stays inside → tracker fires no further events (debounce). (d) Player moves into 體育館 → one event fired with `text=="體育館"`. (e) Player walks into empty space → `Current() == nullptr`, no spurious event this phase. |

`BuildingTracker` tests use `EventBus::Instance().Clear()` + capturing subscribe, matching the pattern in the existing `test_eventbus.cpp`.

## 8. Verification Gate

1. `cmake --build build` with 0 project-code warnings on `-Wall -Wextra -Wpedantic` (raylib internal `_deps/` warnings ignored).
2. `ctest --test-dir build --output-on-failure` all green. Baseline 35 cases + roughly 14 new (5 clamp + 4 bounds + 5 tracker) → ~49 doctest cases.
3. `./build/OOP_Raylib_Lab` survives ≥ 5 s.
4. **Manual visual check (load-bearing):**
   - Walk player up against the worldmap edge — camera stops scrolling; player hit-box stops at the edge; no RayWhite background bleed.
   - Walk the player into 操場 (cx 720, cy 180 in world space) — the HUD shows "進入：操場" and `[Game] Entered: 操場` prints to stdout once.
   - Walk into a different building (e.g. 中正圖書館) — HUD updates, new stdout line.
   - Walk into empty terrain — HUD building name disappears, no stdout spam.
5. `grep -rn 'raylib\.h' src/ tests/ include/ | grep -v 'include/gfx/'` empty.
6. Naming-hygiene grep clean (`claude|codex|claude-code|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md` absent from `git diff main -U0 -- ':(exclude)docs/superpowers'`).
7. Architectural red lines unchanged (Player not including concrete umbrella headers; Item/Umbrella not calling DrawText/DrawTexture; main loop's `objects.erase` after DrawScope close).
8. UTF-8 round-trip: building names from `Buildings.h` print correctly to a UTF-8 terminal (no mojibake).

## 9. Implementation Order Suggestion

Bottom-up TDD layered the same way Phase A was:

1. `gfx/Bounds.h` (`ClampToWorld` free function) + tests.
2. Extend `Camera2D` with `ClampToWorld` method + tests.
3. `WorldConfig.h` (no tests; just constants).
4. `Character::SetPosition` (no new test; covered by tracker tests).
5. `EventBus` — add `EnteredBuilding` enum value (no new test).
6. `Buildings.h` (static array of 27 entries — typed from the Python dict).
7. `BuildingTracker.h` + `BuildingTracker.cpp` + tests.
8. Wire all the above into `src/main.cpp`. Smoke + manual visual.
9. Verification gate.

## 10. Risks & Invariants

- **UTF-8 source consistency:** `Buildings.h` and `main.cpp` must be saved as UTF-8 (no BOM) for Chinese names to roundtrip. macOS Terminal + raylib's `DrawText` default font does not render CJK glyphs, so building names go to `std::cout` (Terminal handles UTF-8 natively) AND `TextBuilder` HUD. The HUD may show boxes for CJK glyphs — that's an asset/font issue parked under Phase D. **Mitigation:** if HUD CJK rendering is unacceptable, fall back to "Entered" (English) + Chinese in stdout only.
- **Coordinate convention:** `triggerRect = {cx - h/2, cy - h/2, h, h}` not the Python `(cx, cy, h)` tuple itself. The plan must transcribe each of the 27 entries with this conversion done — no leaving as TODO.
- **Player size assumption:** `WorldConfig::kPlayerWidth/Height` are constants. If `Player.cpp`'s hit-box ever changes from 24, `WorldConfig` must be updated. The link is brittle but explicit (comment in `Player.cpp` will reference `WorldConfig`).
- **EventBus snapshot rule:** existing snapshot-before-dispatch handler preserves the regression test from `test_eventbus.cpp`. Tracker tests should subscribe to `EnteredBuilding`, not `ShowMessage`, to avoid order coupling.
- **Single-frame transition:** if the player teleports (only via cheats, not in MVP), the tracker would fire once. If two buildings overlap (none currently overlap by inspection of the Python dict), behaviour is the first-match-in-array — document this as "buildings are non-overlapping by registry construction".
- **HUD layout:** new building-name line at (10, 50) does not collide with existing HUD ((10, 10) and (10, 30) with 16-px text leaves the 40–50 px row free).

## 11. Out-of-Scope (parking lot)

- `LeftBuilding` event (and a per-frame `Inside` boolean state) — re-add when chapter quests need it.
- Building interiors / building-scoped chapter switches.
- Visual highlight around the building rect when entered (border / glow).
- Raylib custom CJK font loading — Phase D once a TTF asset is chosen.
- Walkable-region collision (rivers, walls). Phase B clamps only to the worldmap AABB.
- Smooth camera easing.
