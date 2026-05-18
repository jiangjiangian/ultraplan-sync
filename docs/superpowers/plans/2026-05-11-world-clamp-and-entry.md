# Phase B + C — World Bounds Clamp & Building Entry Triggers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clamp camera + player to the 2048×2048 worldmap AABB, materialise a 27-building registry from `tools/composite_worldmap.py`, and detect entry transitions via a `BuildingTracker` that publishes a new `EnteredBuilding` event.

**Architecture:** Pure-data helpers (`Camera2D::ClampToWorld`, free function `gfx::ClampToWorld`) plus a state-tracker class. `Buildings.h` is a `constexpr std::array<27>` typed by hand from the Python source. `EventBus` gains one new enum value. `Player` gains a `SetPosition` mutator (lifted into `Character` for symmetry with `Move`).

**Tech Stack:** C++17, Raylib 5.5, doctest 2.4.11, CMake ≥ 3.14.

**Spec:** `docs/superpowers/specs/2026-05-11-world-clamp-and-entry-design.md`

---

## Project Invariants (apply to every commit)

1. **Wrapper self-containment:** `#include "raylib.h"` only inside `include/gfx/*.h`. Tests and `src/main.cpp` must not include `raylib.h` directly. Verification: `grep -rn 'raylib\.h' src/ tests/ include/ | grep -v 'include/gfx/'` empty (literal `.` escaped — `.` matched a space in Phase A).
2. **No external-tool / agent references in committed content** (code, comments, commit messages, committed markdown): no occurrence of `Claude` / `Codex` / `claude-code` / `superpowers` / `nanobanana` / `Gemini` / `Anthropic` / `AI agent` / `CLAUDE.md` / `AGENTS.md`. Pre-commit grep guard on every commit: `git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md'` must return nothing.
3. **CMake configure flag:** every configure command uses `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
4. **Architectural red lines (must not be crossed):**
   - `Player.h` / `Player.cpp` must not include any concrete `TransparentUmbrella` subclass header.
   - `Item.h`, `Item.cpp`, `TransparentUmbrella.h`, and umbrella subclasses must not call `DrawText` or `DrawTexture`.
   - The main loop's `objects.erase` block must live AFTER the closing brace of the `DrawScope` block (end-of-frame deferred sweep).
5. **UTF-8 source:** all new C++ files saved as UTF-8 (no BOM). Chinese identifiers (building names) appear only as `std::string_view` payload, never as C++ identifiers.

---

## File Structure

New files (8):

| Path | Purpose |
|---|---|
| `include/WorldConfig.h` | `namespace world { kSize, kPlayerWidth, kPlayerHeight }` constants |
| `include/gfx/Bounds.h` | Free function `nccu::gfx::ClampToWorld(pos, size, worldSize) -> Vec2` |
| `include/Buildings.h` | `inline constexpr std::array<Building, 27> kAll` with name + trigger rect |
| `include/BuildingTracker.h` | Class declaration |
| `src/BuildingTracker.cpp` | Implementation (out-of-line so it pulls EventBus.h without leaking into header) |
| `tests/test_bounds.cpp` | 4 cases for `gfx::ClampToWorld` |
| `tests/test_camera2d_clamp.cpp` | 5 cases for `Camera2D::ClampToWorld` |
| `tests/test_building_tracker.cpp` | 5 cases covering the edge-transition state machine |

Modified files (4):

| Path | Change |
|---|---|
| `include/gfx/Camera2D.h` | Add `ClampToWorld(Vec2 worldSize, Vec2 viewportSize) -> Camera2D&` |
| `include/Character.h` | Add `SetPosition(Vec2)` mutator |
| `include/EventBus.h` | Add `EventType::EnteredBuilding` enum value |
| `src/main.cpp` | Wire WorldConfig + Bounds clamp + camera clamp + BuildingTracker + HUD building name + EnteredBuilding subscriber |

---

## Task 0: Baseline build & test in fresh worktree

**Files:** none

- [ ] **Step 1: Confirm worktree branch & base**

Run from worktree root:
```bash
pwd && git rev-parse --abbrev-ref HEAD && git log -1 --oneline
```
Expected: worktree path under `.claude/worktrees/feat+world-clamp-and-entry`, on branch `worktree-feat+world-clamp-and-entry`, HEAD at `22e50bf feat: camera follow + worldmap rendering` (local main with Phase A merged).

If HEAD is `8ab53c6` (the default `EnterWorktree` base from `origin/main`), rebase first:
```bash
git rebase main
```
Then re-verify HEAD.

- [ ] **Step 2: Ensure resources symlink exists**

The worldmap PNG lives in untracked main-repo `resources/assets/`. For the smoke test in Task 9 to succeed, the worktree needs the same files reachable from `resources/assets/maps/worldmap.png`.

```bash
[ -L resources/assets ] || ln -s /Users/ian/Desktop/assignment-5-jiangjiangian/resources/assets resources/assets
ls resources/assets/maps/worldmap.png
```
Expected: file exists.

- [ ] **Step 3: Configure CMake**

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```
Expected: ends with `-- Configuring done` and `-- Generating done`.

- [ ] **Step 4: Baseline build + tests**

```bash
cmake --build build 2>&1 | tail -3
ctest --test-dir build --output-on-failure
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero project-code warnings (raylib `_deps/` ignored), ctest passes (`unit_tests`), `[doctest] test cases: 35 | 35 passed`.

If anything fails: STOP and surface the failure. Do not proceed.

---

## Task 1: `gfx::ClampToWorld` free helper + tests

**Files:**
- Create: `include/gfx/Bounds.h`
- Create: `tests/test_bounds.cpp`

- [ ] **Step 1: Write the failing test first**

Write `tests/test_bounds.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/Bounds.h"
#include "gfx/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("ClampToWorld: point inside world is unchanged") {
    Vec2 p = ClampToWorld(Vec2{100.0f, 100.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(p.x == doctest::Approx(100.0f));
    CHECK(p.y == doctest::Approx(100.0f));
}

TEST_CASE("ClampToWorld: negative position pins to 0") {
    Vec2 p = ClampToWorld(Vec2{-50.0f, -50.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(p.x == doctest::Approx(0.0f));
    CHECK(p.y == doctest::Approx(0.0f));
}

TEST_CASE("ClampToWorld: position past lower-right pins so AABB stays in world") {
    Vec2 p = ClampToWorld(Vec2{3000.0f, 3000.0f}, Vec2{24.0f, 24.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(p.x == doctest::Approx(2048.0f - 24.0f));
    CHECK(p.y == doctest::Approx(2048.0f - 24.0f));
}

TEST_CASE("ClampToWorld: size larger than world pins position to 0") {
    Vec2 p = ClampToWorld(Vec2{500.0f, 500.0f}, Vec2{3000.0f, 3000.0f}, Vec2{2048.0f, 2048.0f});
    CHECK(p.x == doctest::Approx(0.0f));
    CHECK(p.y == doctest::Approx(0.0f));
}
```

- [ ] **Step 2: Verify the test fails to build**

```bash
cmake --build build 2>&1 | grep -E 'fatal error.*Bounds\.h' | head -3
```
Expected: `fatal error: 'gfx/Bounds.h' file not found`.

- [ ] **Step 3: Create `include/gfx/Bounds.h`**

```cpp
#pragma once
#include "gfx/Vec2.h"
#include <algorithm>

namespace nccu::gfx {

// Returns pos clamped so a `size`-px AABB anchored at pos stays inside
// the [0, worldSize] AABB. If size > worldSize on an axis, pos pins at 0
// on that axis.
inline Vec2 ClampToWorld(Vec2 pos, Vec2 size, Vec2 worldSize) noexcept {
    const float maxX = worldSize.x - size.x;
    const float maxY = worldSize.y - size.y;
    const float clampedX = maxX < 0.0f ? 0.0f : std::clamp(pos.x, 0.0f, maxX);
    const float clampedY = maxY < 0.0f ? 0.0f : std::clamp(pos.y, 0.0f, maxY);
    return Vec2{clampedX, clampedY};
}

} // namespace nccu::gfx
```

- [ ] **Step 4: Re-configure (new test file) + build + test**

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 2>&1 | tail -3
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero project warnings; test case count is now 39 (35 + 4 new); all green.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Bounds.h tests/test_bounds.cpp
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(gfx): add ClampToWorld free helper for AABB-in-world clamp"
```

---

## Task 2: `Camera2D::ClampToWorld` method + tests

**Files:**
- Modify: `include/gfx/Camera2D.h`
- Create: `tests/test_camera2d_clamp.cpp`

- [ ] **Step 1: Write the failing test**

Write `tests/test_camera2d_clamp.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/Camera2D.h"
#include "gfx/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("Camera2D::ClampToWorld: target away from edges is unchanged") {
    Camera2D c;
    c.Follow(Vec2{1024.0f, 1024.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(1024.0f));
    CHECK(c.target.y == doctest::Approx(1024.0f));
}

TEST_CASE("Camera2D::ClampToWorld: target near upper-left clamps to viewport/2") {
    Camera2D c;
    c.Follow(Vec2{0.0f, 0.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(400.0f));
    CHECK(c.target.y == doctest::Approx(225.0f));
}

TEST_CASE("Camera2D::ClampToWorld: target near lower-right clamps to world - viewport/2") {
    Camera2D c;
    c.Follow(Vec2{2048.0f, 2048.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(2048.0f - 400.0f));
    CHECK(c.target.y == doctest::Approx(2048.0f - 225.0f));
}

TEST_CASE("Camera2D::ClampToWorld: when world smaller than viewport target pins to world center") {
    Camera2D c;
    c.Follow(Vec2{100.0f, 100.0f}, Vec2{400.0f, 225.0f});
    c.ClampToWorld(Vec2{200.0f, 200.0f}, Vec2{800.0f, 450.0f});
    CHECK(c.target.x == doctest::Approx(100.0f));
    CHECK(c.target.y == doctest::Approx(100.0f));
}

TEST_CASE("Camera2D::ClampToWorld is fluent: returns *this") {
    Camera2D c;
    auto& ret = c.ClampToWorld(Vec2{2048.0f, 2048.0f}, Vec2{800.0f, 450.0f});
    CHECK(&ret == &c);
}
```

- [ ] **Step 2: Verify the test fails to build**

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 2>&1 | tail -3
cmake --build build 2>&1 | grep -E "'ClampToWorld'" | head -3
```
Expected: error about `ClampToWorld` not being a member of `Camera2D`.

- [ ] **Step 3: Add `ClampToWorld` to `include/gfx/Camera2D.h`**

Insert the following method inside the `Camera2D` struct (between `WithRotation` and the closing brace):
```cpp
    // Clamp .target so a viewport of `viewportSize` centred on .target
    // stays inside [0, worldSize] on both axes. If the world is smaller
    // than the viewport on an axis, .target pins to the world centre on
    // that axis.
    Camera2D& ClampToWorld(Vec2 worldSize, Vec2 viewportSize) noexcept {
        const float halfW = viewportSize.x * 0.5f;
        const float halfH = viewportSize.y * 0.5f;
        if (worldSize.x <= viewportSize.x) {
            target.x = worldSize.x * 0.5f;
        } else if (target.x < halfW) {
            target.x = halfW;
        } else if (target.x > worldSize.x - halfW) {
            target.x = worldSize.x - halfW;
        }
        if (worldSize.y <= viewportSize.y) {
            target.y = worldSize.y * 0.5f;
        } else if (target.y < halfH) {
            target.y = halfH;
        } else if (target.y > worldSize.y - halfH) {
            target.y = worldSize.y - halfH;
        }
        return *this;
    }
```

- [ ] **Step 4: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero project warnings; doctest count now 44 (39 + 5); all green.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Camera2D.h tests/test_camera2d_clamp.cpp
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(gfx): add Camera2D::ClampToWorld for viewport-in-world AABB clamp"
```

---

## Task 3: `WorldConfig.h` constants

**Files:**
- Create: `include/WorldConfig.h`

No new test — these are constants consumed by Task 9 wiring.

- [ ] **Step 1: Create `include/WorldConfig.h`**

```cpp
#pragma once

namespace world {

// 2048x2048 NCCU 山下 worldmap, matches resources/assets/maps/worldmap.png.
inline constexpr float kSize         = 2048.0f;

// Mirrors the 24-px square hit-box constructed in Player.cpp's ctor.
// If Player's hit-box ever changes from 24, update both here AND in
// src/Player.cpp at the same time.
inline constexpr float kPlayerWidth  = 24.0f;
inline constexpr float kPlayerHeight = 24.0f;

} // namespace world
```

- [ ] **Step 2: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero warnings; doctest still at 44 (no new tests yet).

- [ ] **Step 3: Commit**

```bash
git add include/WorldConfig.h
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(core): add WorldConfig constants for world size and player size"
```

---

## Task 4: `Character::SetPosition` mutator

**Files:**
- Modify: `include/Character.h`

- [ ] **Step 1: Add `SetPosition` to `Character.h`**

Insert this method inside the `Character` class, after the `Move` method (before `protected:`):
```cpp
    // Symmetric with GetPosition (from GameObject). Keeps hitBox_ in
    // lock-step with position_, the same invariant Move maintains.
    void SetPosition(nccu::gfx::Vec2 p) noexcept {
        position_ = p;
        hitBox_.x = p.x;
        hitBox_.y = p.y;
    }
```

- [ ] **Step 2: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero warnings; doctest still at 44.

- [ ] **Step 3: Commit**

```bash
git add include/Character.h
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(core): add Character::SetPosition mutator with hit-box sync"
```

---

## Task 5: `EnteredBuilding` event type

**Files:**
- Modify: `include/EventBus.h`

- [ ] **Step 1: Add the enum value**

Locate the `EventType` enum in `include/EventBus.h` and add `EnteredBuilding` after `ShowMessage`:
```cpp
enum class EventType {
    RenderRequested,
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
    EnteredBuilding,
};
```

- [ ] **Step 2: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero warnings; doctest still at 44.

- [ ] **Step 3: Commit**

```bash
git add include/EventBus.h
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(observer): add EventType::EnteredBuilding"
```

---

## Task 6: `Buildings.h` registry of 27 NCCU buildings

**Files:**
- Create: `include/Buildings.h`

No new test for the registry data itself; downstream tracker tests exercise the array indirectly.

- [ ] **Step 1: Create `include/Buildings.h`**

Coordinates are transcribed from `tools/composite_worldmap.py`'s `BUILDINGS` dict, applying the transform `triggerRect = {cx - h/2, cy - h/2, h, h}`.

```cpp
#pragma once
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::buildings {

struct Building {
    std::string_view name;
    nccu::gfx::Rect  triggerRect;  // world-space AABB
};

// Order matches the BUILDINGS dict iteration order in
// tools/composite_worldmap.py. Each entry's triggerRect is a centred
// square whose side equals the painted building's target height; this
// is intentionally slightly oversized so the player can step into the
// trigger from any direction.
inline constexpr std::array<Building, 27> kAll = {{
    {"操場",        nccu::gfx::Rect{ 720.0f,  180.0f, 360.0f, 360.0f}},
    {"體育館",      nccu::gfx::Rect{1320.0f,  230.0f, 260.0f, 260.0f}},
    {"游泳館",      nccu::gfx::Rect{1530.0f,  490.0f, 220.0f, 220.0f}},
    {"研究大樓",    nccu::gfx::Rect{ 150.0f,  180.0f, 220.0f, 220.0f}},
    {"樂活館",      nccu::gfx::Rect{ 200.0f,  500.0f, 200.0f, 200.0f}},
    {"樂活小舖",    nccu::gfx::Rect{ 430.0f,  630.0f, 180.0f, 180.0f}},
    {"羅馬廣場",    nccu::gfx::Rect{ 760.0f,  780.0f, 280.0f, 280.0f}},
    {"中正圖書館",  nccu::gfx::Rect{ 325.0f,  875.0f, 290.0f, 290.0f}},
    {"綜合院館",    nccu::gfx::Rect{1170.0f,  870.0f, 280.0f, 280.0f}},
    {"校友服務中心", nccu::gfx::Rect{1550.0f,  970.0f, 220.0f, 220.0f}},
    {"正門",        nccu::gfx::Rect{  30.0f,  880.0f, 200.0f, 200.0f}},
    {"行政大樓",    nccu::gfx::Rect{ 200.0f, 1200.0f, 280.0f, 280.0f}},
    {"商學院",      nccu::gfx::Rect{ 560.0f, 1180.0f, 240.0f, 240.0f}},
    {"新聞館",      nccu::gfx::Rect{ 410.0f, 1410.0f, 240.0f, 240.0f}},
    {"資訊大樓",    nccu::gfx::Rect{ 710.0f, 1380.0f, 240.0f, 240.0f}},
    {"法學院",      nccu::gfx::Rect{ 110.0f, 1400.0f, 200.0f, 200.0f}},
    {"風雩樓",      nccu::gfx::Rect{ 960.0f, 1370.0f, 240.0f, 240.0f}},
    {"風雩走廊",    nccu::gfx::Rect{1180.0f, 1520.0f, 200.0f, 200.0f}},
    {"大仁樓",      nccu::gfx::Rect{1370.0f, 1160.0f, 240.0f, 240.0f}},
    {"大智樓",      nccu::gfx::Rect{1590.0f, 1210.0f, 220.0f, 220.0f}},
    {"大勇樓",      nccu::gfx::Rect{1380.0f, 1410.0f, 220.0f, 220.0f}},
    {"學思樓",      nccu::gfx::Rect{1600.0f, 1460.0f, 220.0f, 220.0f}},
    {"志希樓",      nccu::gfx::Rect{1380.0f, 1620.0f, 220.0f, 220.0f}},
    {"集英樓",      nccu::gfx::Rect{1140.0f, 1630.0f, 260.0f, 260.0f}},
    {"果夫樓",      nccu::gfx::Rect{ 970.0f, 1630.0f, 220.0f, 220.0f}},
    {"四維堂",      nccu::gfx::Rect{ 710.0f, 1600.0f, 240.0f, 240.0f}},
    {"井塘樓",      nccu::gfx::Rect{ 500.0f, 1610.0f, 220.0f, 220.0f}},
}};

} // namespace nccu::buildings
```

- [ ] **Step 2: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero warnings; doctest still at 44.

- [ ] **Step 3: Commit**

```bash
git add include/Buildings.h
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(world): add 27-building static registry with trigger rects"
```

---

## Task 7: `BuildingTracker` + tests

**Files:**
- Create: `include/BuildingTracker.h`
- Create: `src/BuildingTracker.cpp`
- Create: `tests/test_building_tracker.cpp`

- [ ] **Step 1: Write the failing test**

Write `tests/test_building_tracker.cpp`:
```cpp
#include "doctest/doctest.h"
#include "BuildingTracker.h"
#include "Buildings.h"
#include "EventBus.h"
#include "gfx/Vec2.h"

#include <string>

TEST_CASE("BuildingTracker: starts with no current building") {
    EventBus::Instance().Clear();
    nccu::BuildingTracker t;
    CHECK(t.Current() == nullptr);
}

TEST_CASE("BuildingTracker: entering a building publishes EnteredBuilding with name") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event& e) { hits++; captured = e.text; });

    nccu::BuildingTracker t;
    // 操場 trigger is {720, 180, 360, 360} — centre at (900, 360).
    auto* b = t.Update(nccu::gfx::Vec2{900.0f, 360.0f});

    REQUIRE(b != nullptr);
    CHECK(b->name == "操場");
    CHECK(hits == 1);
    CHECK(captured == "操場");
}

TEST_CASE("BuildingTracker: staying inside the same building does not re-publish") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event&) { hits++; });

    nccu::BuildingTracker t;
    t.Update(nccu::gfx::Vec2{900.0f, 360.0f});  // enter 操場
    t.Update(nccu::gfx::Vec2{905.0f, 365.0f});  // still inside 操場
    t.Update(nccu::gfx::Vec2{890.0f, 350.0f});  // still inside 操場
    CHECK(hits == 1);
}

TEST_CASE("BuildingTracker: moving to a different building publishes again") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string lastName;
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event& e) { hits++; lastName = e.text; });

    nccu::BuildingTracker t;
    t.Update(nccu::gfx::Vec2{900.0f, 360.0f});   // enter 操場
    // 體育館 trigger is {1320, 230, 260, 260} — centre at (1450, 360).
    auto* b = t.Update(nccu::gfx::Vec2{1450.0f, 360.0f});

    REQUIRE(b != nullptr);
    CHECK(b->name == "體育館");
    CHECK(hits == 2);
    CHECK(lastName == "體育館");
}

TEST_CASE("BuildingTracker: leaving a building clears current with no spurious event") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&](const Event&) { hits++; });

    nccu::BuildingTracker t;
    t.Update(nccu::gfx::Vec2{900.0f, 360.0f});   // enter 操場
    auto* none = t.Update(nccu::gfx::Vec2{1024.0f, 600.0f});  // empty terrain

    CHECK(none == nullptr);
    CHECK(t.Current() == nullptr);
    CHECK(hits == 1);  // only the initial enter, no leave-event in this phase
}
```

- [ ] **Step 2: Verify the test fails to build**

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 2>&1 | tail -3
cmake --build build 2>&1 | grep -E 'fatal error.*BuildingTracker' | head -3
```
Expected: `fatal error: 'BuildingTracker.h' file not found`.

- [ ] **Step 3: Create `include/BuildingTracker.h`**

```cpp
#pragma once
#include "Buildings.h"
#include "gfx/Vec2.h"

namespace nccu {

class BuildingTracker {
public:
    // Called once per frame with the player's centre point in world
    // coordinates. Returns a pointer to the building the player is
    // INSIDE this frame (nullptr if none). On a transition from one
    // current building to another (or from nullptr to non-null), an
    // EventType::EnteredBuilding event is published carrying the new
    // building's name. No event is published when leaving a building
    // into empty terrain in this phase.
    const buildings::Building* Update(gfx::Vec2 playerCenter);

    const buildings::Building* Current() const noexcept { return current_; }

private:
    const buildings::Building* current_{nullptr};
};

} // namespace nccu
```

- [ ] **Step 4: Create `src/BuildingTracker.cpp`**

```cpp
#include "BuildingTracker.h"
#include "EventBus.h"
#include "gfx/Color.h"

#include <string>

namespace nccu {

const buildings::Building* BuildingTracker::Update(gfx::Vec2 playerCenter) {
    const buildings::Building* found = nullptr;
    for (const auto& b : buildings::kAll) {
        if (b.triggerRect.Contains(playerCenter)) {
            found = &b;
            break;
        }
    }

    if (found != current_) {
        current_ = found;
        if (found) {
            const auto& r = found->triggerRect;
            EventBus::Instance().Publish(Event{
                EventType::EnteredBuilding,
                gfx::Vec2{r.x + r.width * 0.5f, r.y + r.height * 0.5f},
                gfx::Colors::White,
                std::string{found->name}
            });
        }
    }
    return current_;
}

} // namespace nccu
```

- [ ] **Step 5: Build + test**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: zero warnings; doctest count now 49 (44 + 5 new); all green.

- [ ] **Step 6: Commit**

```bash
git add include/BuildingTracker.h src/BuildingTracker.cpp tests/test_building_tracker.cpp
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(world): add BuildingTracker single-edge state machine"
```

---

## Task 8: Wire everything into `src/main.cpp`

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace `src/main.cpp` with the wired version**

Overwrite the entire file with:
```cpp
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"
#include "BuildingTracker.h"
#include "Buildings.h"
#include "WorldConfig.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/CameraScope.h"
#include "gfx/Camera2D.h"
#include "gfx/Texture.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include "gfx/Bounds.h"
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <string>

int main() {
    using namespace nccu::gfx;

    constexpr int kWinW = 800;
    constexpr int kWinH = 450;
    const Vec2    kScreenCenter{kWinW / 2.0f, kWinH / 2.0f};
    const Vec2    kWorldSize{world::kSize, world::kSize};
    const Vec2    kViewport{static_cast<float>(kWinW), static_cast<float>(kWinH)};
    const Vec2    kPlayerSize{world::kPlayerWidth, world::kPlayerHeight};

    auto win = Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    auto worldmap = nccu::gfx::Texture::Load("resources/assets/maps/worldmap.png");

    // Mutable HUD state — current building name shown next frame.
    std::string currentBuildingName;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            Renderer{}.Rect(Rect{e.position.x, e.position.y, 20, 20}, e.color);
        });
    EventBus::Instance().Subscribe(EventType::EnteredBuilding,
        [&currentBuildingName](const Event& e) {
            std::cout << "[Game] Entered: " << e.text << '\n';
            currentBuildingName = e.text;
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{400, 225}));
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{150, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{300, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{500, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{650, 100}));

    Player* player = dynamic_cast<Player*>(objects.front().get());
    nccu::gfx::Camera2D cam;
    nccu::BuildingTracker tracker;

    while (!win.ShouldClose()) {
        float dt = Time::DeltaSeconds();

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
        }

        // Phase B: clamp player's world position so the hit-box stays
        // inside the worldmap AABB.
        if (player) {
            player->SetPosition(ClampToWorld(player->GetPosition(), kPlayerSize, kWorldSize));
        }

        if (Input::IsPressed(Key::E) && player) {
            for (auto& obj : objects) {
                if (!obj || !obj->IsActive() || obj.get() == player) continue;
                Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
                if (obj->CheckCollision(pHit)) {
                    obj->Interact(player);
                }
            }
        }

        // Phase C: building-entry edge detection. Use player CENTRE
        // (position + half-size) as the test point so a 24-px player
        // hits the trigger when their middle crosses the rect edge.
        if (player) {
            const Vec2 p = player->GetPosition();
            const Vec2 center{p.x + kPlayerSize.x * 0.5f, p.y + kPlayerSize.y * 0.5f};
            auto* hit = tracker.Update(center);
            if (!hit) currentBuildingName.clear();
        }

        if (player) {
            cam.Follow(player->GetPosition(), kScreenCenter)
               .ClampToWorld(kWorldSize, kViewport);
        }

        {
            DrawScope frame;
            Renderer{}.Clear(Colors::RayWhite);

            // World-space pass: worldmap + game objects scroll with the camera.
            {
                CameraScope view{cam};
                Renderer{}.Texture(worldmap, Vec2{0.0f, 0.0f});
                for (auto& obj : objects) {
                    if (obj && obj->IsActive()) obj->Draw();
                }
            }

            // Screen-space HUD.
            TextBuilder{"WASD: move    E: pick up"}
                .At(Vec2{10, 10}).Size(16).Color(Colors::DarkGray).Draw();
            if (player) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "karma: %d   umbrella: %s",
                    player->GetKarma(), player->HasUmbrella() ? "yes" : "no");
                TextBuilder{buf}
                    .At(Vec2{10, 30}).Size(16).Color(Colors::DarkGray).Draw();
            }
            if (!currentBuildingName.empty()) {
                const std::string line = "Inside: " + currentBuildingName;
                TextBuilder{line}
                    .At(Vec2{10, 50}).Size(16).Color(Colors::Black).Draw();
            }
        }

        // End-of-frame sweep: deferred deletion to avoid iterator invalidation.
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [](const std::unique_ptr<GameObject>& o) {
                    return !o || !o->IsActive();
                }),
            objects.end());
        if (player && std::find_if(objects.begin(), objects.end(),
                [player](const std::unique_ptr<GameObject>& o) { return o.get() == player; })
            == objects.end()) {
            player = nullptr;
        }
    }

    return 0;
}
```

Key change rationale:
- Player clamp happens BEFORE input interaction, so the hit-box used for E-key collision is already inside world bounds.
- Tracker uses player CENTRE (not corner) so the trigger rect engages when the player's middle enters it.
- Camera follow is chained `.Follow(...).ClampToWorld(...)` to demonstrate the fluent API.
- HUD building line uses ASCII prefix `"Inside: "` because raylib's default font lacks CJK glyphs; the Chinese name will render as fallback boxes but is preserved verbatim. (Acceptable for MVP; full CJK font is parked for a later phase.)
- `currentBuildingName.clear()` runs when tracker returns nullptr, so the HUD line disappears as soon as the player walks out into empty terrain.

- [ ] **Step 2: Build**

```bash
cmake --build build 2>&1 | grep -E ': (warning|error):' | grep -v '_deps/' | head -10
```
Expected: zero project warnings.

- [ ] **Step 3: Run all tests**

```bash
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: 49 cases, all green.

- [ ] **Step 4: 5-second smoke**

```bash
./build/OOP_Raylib_Lab > /tmp/phaseBC-smoke.log 2>&1 &
PID=$!
sleep 6
if kill -0 "$PID" 2>/dev/null; then
    echo SURVIVED
    kill "$PID" 2>/dev/null
    wait "$PID" 2>/dev/null
else
    echo CRASHED
fi
tail -10 /tmp/phaseBC-smoke.log
```
Expected: `SURVIVED`. The log tail should include `FILEIO: [resources/assets/maps/worldmap.png] File loaded successfully` and `IMAGE: Data loaded successfully (2048x2048 ...)`.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md' && echo BLOCKED || echo CLEAR
git commit -m "feat(game): clamp player + camera to world AABB and detect building entry"
```

---

## Task 9: Verification gate

**Files:** none (verification only)

- [ ] **Step 1: Clean rebuild**

```bash
rm -rf build
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 2>&1 | tail -3
cmake --build build 2>&1 | tee /tmp/phaseBC-build.log | tail -5
```
Expected: build succeeds.

- [ ] **Step 2: Zero project-code warnings**

```bash
grep -E ': (warning|error):' /tmp/phaseBC-build.log | grep -v '_deps/' | head -10
```
Expected: no output.

- [ ] **Step 3: Full test suite**

```bash
ctest --test-dir build --output-on-failure
./build/OOP_Raylib_Lab_tests --reporters=console 2>&1 | tail -3
```
Expected: `100% tests passed`; `[doctest] test cases: 49 | 49 passed`.

- [ ] **Step 4: 5-second smoke**

```bash
./build/OOP_Raylib_Lab > /tmp/phaseBC-smoke2.log 2>&1 &
PID=$!
sleep 6
if kill -0 "$PID" 2>/dev/null; then
    echo SURVIVED
    kill "$PID" 2>/dev/null
    wait "$PID" 2>/dev/null
else
    echo CRASHED
fi
```
Expected: `SURVIVED`.

- [ ] **Step 5: raylib.h self-containment grep (literal-dot)**

```bash
grep -rn 'raylib\.h' src/ tests/ include/ | grep -v 'include/gfx/'
```
Expected: no output.

- [ ] **Step 6: Architectural red-line greps**

```bash
echo "=== rule 1: Player must not include concrete TransparentUmbrella subclass header ==="
grep -nE '#include\s+"(TrueUmbrella|FragileUmbrella|ProfessorTrapUmbrella|CursedUmbrella)\.h"' include/Player.h src/Player.cpp || echo "(clean)"

echo "=== rule 2: Item / Umbrella family must not call DrawText/DrawTexture ==="
grep -nE 'Draw(Text|Texture)' include/Item.h include/TransparentUmbrella.h src/TrueUmbrella.cpp src/FragileUmbrella.cpp src/ProfessorTrapUmbrella.cpp src/CursedUmbrella.cpp 2>/dev/null || echo "(clean)"

echo "=== rule 3: main loop erase block is AFTER DrawScope closes ==="
grep -n 'objects.erase' src/main.cpp
# Verify visually that the erase line sits after the line that closes the DrawScope block.
```
Expected: all three sections clean.

- [ ] **Step 7: AI/tool mention grep against main**

```bash
git diff main -U0 -- ':(exclude)docs/superpowers' | \
    grep -Ei 'claude|codex|claude-code|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md'
```
Expected: no output.

- [ ] **Step 8: Confirm 5 commits ahead of main**

```bash
git log --oneline main..HEAD
```
Expected: 8 commits (Tasks 1, 2, 3, 4, 5, 6, 7, 8).

- [ ] **Step 9: Stop here**

Do **NOT** proceed to merge. Print the following summary and exit:
```
Phase B + C complete on worktree-feat+world-clamp-and-entry, 8 commits ahead of main.
Build clean, 49 doctest cases pass, 5s smoke survives. raylib.h confined to include/gfx/, architectural red lines untouched, no AI/tool references in diff.

Manual visual check pending (human):
  - Walk player up to worldmap edge → camera stops scrolling, no RayWhite bleed.
  - Walk into 操場 (world 900,360) → stdout prints "[Game] Entered: 操場", HUD shows "Inside: <name>".
  - Walk to a different building → HUD updates, new stdout line.
  - Walk into empty terrain → HUD line disappears, no spurious events.

User to decide finishing option (squash-merge / fast-forward / keep branch). No push (per project memory).
```

---

## Manual visual checklist (sticky reference)

When the user runs `./build/OOP_Raylib_Lab` from the worktree root:

| Verification | Expected |
|---|---|
| Walk up to top edge (y → 0) | Camera stops scrolling; player rect stops at edge; no RayWhite bleed beyond worldmap |
| Walk into 操場 trigger (around world 900, 360) | stdout: `[Game] Entered: 操場`; HUD line "Inside: <fallback-glyphs>" appears |
| Walk into 體育館 (around world 1450, 360) | stdout: `[Game] Entered: 體育館`; HUD line updates to new name |
| Walk back into empty terrain | HUD line disappears, no spurious stdout |
| Stay still inside a building | No event spam — single line of stdout per entry |

---

## Self-Review Notes (plan author)

Spec coverage:
- §4.1 WorldConfig → Task 3 ✓
- §4.2 Camera clamp → Task 2 ✓
- §4.3 Player clamp helper → Task 1 ✓
- §4.4 Buildings registry → Task 6 ✓
- §4.5 BuildingTracker → Task 7 ✓
- §4.6 EnteredBuilding event → Task 5 ✓
- §4.7 SetPosition → Task 4 ✓
- §4.8 main.cpp wiring → Task 8 ✓
- §7 tests → bounds (T1), clamp (T2), tracker (T7) — three test files matching the three test sections in the spec ✓
- §8 verification gate — Task 9 mirrors all eight items ✓

Type consistency:
- `Vec2`, `Rect`, `Color`, `Camera2D` come from existing `nccu::gfx` namespace.
- `Building` and `kAll` in `nccu::buildings` namespace per spec §4.4.
- `BuildingTracker` in `nccu` namespace per spec §4.5.
- `Event::text` is `std::string`, constructed from `std::string_view{building name}` via `std::string{sv}` — already done in BuildingTracker.cpp.

Intentional simplifications (documented in plan):
- Player clamp is rectangle-AABB, not walkable-region — full pixel mask is parked for Phase D.
- LeftBuilding event is not implemented in this phase; HUD line clears via per-frame `currentBuildingName.clear()` when tracker returns nullptr.
- HUD prefix is `"Inside: "` ASCII so the line is readable; the Chinese name will fall back to box glyphs in raylib's default font. Console output is fully UTF-8.
