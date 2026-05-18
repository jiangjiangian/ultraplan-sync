# Camera + Worldmap (Phase A) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the player roam a pre-generated 2048×2048 NCCU 山下 worldmap with the camera following along, on top of the existing `nccu::gfx` wrapper layer.

**Architecture:** Header-only additions under `include/gfx/`: a move-only `Texture` RAII handle, a `Camera2D` adapter struct with fluent setters, a `CameraScope` RAII pairing `BeginMode2D`/`EndMode2D`, and a `Renderer::Texture(...)` draw helper. `src/main.cpp` loads `resources/assets/maps/worldmap.png` once at startup, runs a single `CameraScope` per frame for world-space draws (worldmap + GameObjects), and renders HUD outside the scope so it stays pinned to screen.

**Tech Stack:** C++17, Raylib 5.5 (fetched via CMake FetchContent), doctest 2.4.11, CMake ≥ 3.14.

**Spec:** `docs/superpowers/specs/2026-05-11-camera-worldmap-design.md`

---

## Project Invariants (apply to every task)

1. **Wrapper self-containment:** `#include "raylib.h"` may appear only inside `include/gfx/*.h`. New files `Texture.h`, `Camera2D.h`, `CameraScope.h` are inside `include/gfx/` and may include `raylib.h`; `src/main.cpp` and `tests/test_camera2d.cpp` must not. Verification: `grep -rn 'raylib.h' src/ tests/ include/ | grep -v 'include/gfx/'` returns no lines.
2. **No external-tool / agent references in committed content:** code, comments, commit messages, and committed markdown must not mention Claude / Codex / claude-code / superpowers / nanobanana / Gemini / Anthropic / "AI agent" / CLAUDE.md / AGENTS.md. Pre-commit grep before each Step "Commit": `git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md'` must return nothing.
3. **CMake configure flag:** every configure command in this plan uses `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`. Raylib's nested `cmake_minimum_required(VERSION 3.0)` is below the floor modern CMake (4.x) accepts without this flag.
4. **Architectural red lines (not touched by Phase A, but the verification step greps to confirm):**
   - `Player.h` / `Player.cpp` must not include any concrete `TransparentUmbrella` subclass header.
   - `Item.h`, `Item.cpp`, `TransparentUmbrella.h`, and umbrella subclasses must not call `DrawText` or `DrawTexture`.
   - The main loop must not delete entries from `objects` mid-iteration; sweep stays end-of-frame.

---

## File Structure

New files (5):

| Path | Purpose |
|---|---|
| `include/gfx/Camera2D.h` | Adapter struct: `offset`, `target`, `rotation`, `zoom` + fluent `Follow / WithZoom / WithRotation` |
| `include/gfx/Texture.h` | Move-only RAII handle around `::Texture2D`; static `Texture::Load(path)` factory |
| `include/gfx/CameraScope.h` | RAII pairing `BeginMode2D` / `EndMode2D`; non-copyable, non-movable |
| `tests/test_camera2d.cpp` | doctest cases covering Camera2D defaults, `Follow`, `WithZoom`, `WithRotation`, fluent return |
| (none other) | |

Modified files (2):

| Path | Change |
|---|---|
| `include/gfx/Renderer.h` | Add `Renderer& Texture(const class Texture& tex, Vec2 pos, Color tint = Colors::White)` |
| `src/main.cpp` | Load worldmap; construct `Camera2D`; wrap world-space draws in `CameraScope`; keep HUD outside |

**Naming note:** The spec sketch had `Camera2D::Zoom(z)` as a method. C++ does not allow a class member function and data member to share a name in the same scope, and the field `zoom` is named to match raylib's `::Camera2D` for readability. The fluent setters in this plan are named `WithZoom` / `WithRotation`; `Follow(target, screenCenter)` remains as the spec defined. Tests assert via the public fields (`c.zoom == 2.0f`, etc.).

---

## Task 0: Baseline build & test in fresh worktree

**Files:** none

- [ ] **Step 1: Verify you're in the new worktree (branch `feat/camera-worldmap`)**

Run: `git rev-parse --abbrev-ref HEAD`
Expected output: `feat/camera-worldmap` (or whatever branch the worktree was created with).

- [ ] **Step 2: Configure CMake**

Run from worktree root:

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Expected: configure succeeds; final lines include `-- Configuring done` and `-- Generating done`.

- [ ] **Step 3: Build**

Run: `cmake --build build`
Expected: build succeeds, zero warnings, produces `build/OOP_Raylib_Lab` and `build/OOP_Raylib_Lab_tests`.

- [ ] **Step 4: Run existing tests as baseline**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed, 0 tests failed out of 1` (single ctest target `unit_tests` aggregates all doctest cases).

If any of the above fails: stop, surface the failure, do not proceed.

---

## Task 1: `Camera2D` adapter struct + tests

**Files:**
- Create: `include/gfx/Camera2D.h`
- Create: `tests/test_camera2d.cpp`

- [ ] **Step 1: Write the failing test**

Write `tests/test_camera2d.cpp`:

```cpp
#include "doctest/doctest.h"
#include "gfx/Camera2D.h"
#include "gfx/Vec2.h"

using namespace nccu::gfx;

TEST_CASE("Camera2D defaults: zero offset/target/rotation, zoom 1.0") {
    Camera2D c;
    CHECK(c.offset.x   == doctest::Approx(0.0f));
    CHECK(c.offset.y   == doctest::Approx(0.0f));
    CHECK(c.target.x   == doctest::Approx(0.0f));
    CHECK(c.target.y   == doctest::Approx(0.0f));
    CHECK(c.rotation   == doctest::Approx(0.0f));
    CHECK(c.zoom       == doctest::Approx(1.0f));
}

TEST_CASE("Camera2D::Follow sets target = world target and offset = screen center") {
    Camera2D c;
    auto& ret = c.Follow(Vec2{1000.0f, 500.0f}, Vec2{400.0f, 225.0f});
    CHECK(&ret == &c);                            // fluent: returns *this
    CHECK(c.target.x == doctest::Approx(1000.0f));
    CHECK(c.target.y == doctest::Approx(500.0f));
    CHECK(c.offset.x == doctest::Approx(400.0f));
    CHECK(c.offset.y == doctest::Approx(225.0f));
}

TEST_CASE("Camera2D::WithZoom + WithRotation are fluent setters") {
    Camera2D c;
    auto& ret = c.WithZoom(2.0f).WithRotation(45.0f);
    CHECK(&ret == &c);
    CHECK(c.zoom     == doctest::Approx(2.0f));
    CHECK(c.rotation == doctest::Approx(45.0f));
}
```

- [ ] **Step 2: Build and verify the test fails to compile**

Run: `cmake --build build`
Expected: build fails with a "no such file" or "Camera2D not declared" error referencing `gfx/Camera2D.h`. This proves the test wires up.

- [ ] **Step 3: Create `include/gfx/Camera2D.h`**

```cpp
#pragma once
#include "gfx/Vec2.h"

namespace nccu::gfx {

// Adapter for raylib's ::Camera2D. We keep field names matching raylib
// (offset, target, rotation, zoom) for readability, and expose fluent
// setters for in-frame chaining: cam.Follow(p, center).WithZoom(1.0f);
struct Camera2D {
    Vec2  offset{0.0f, 0.0f};
    Vec2  target{0.0f, 0.0f};
    float rotation{0.0f};
    float zoom{1.0f};

    Camera2D& Follow(Vec2 worldTarget, Vec2 screenCenter) noexcept {
        target = worldTarget;
        offset = screenCenter;
        return *this;
    }
    Camera2D& WithZoom(float z) noexcept     { zoom = z;     return *this; }
    Camera2D& WithRotation(float r) noexcept { rotation = r; return *this; }
};

} // namespace nccu::gfx
```

(Note: this header deliberately does **not** include `raylib.h` — it is pure data. `CameraScope` will do the raylib conversion at the boundary.)

- [ ] **Step 4: Build and run tests**

Run: `cmake --build build && ctest --test-dir build --output-on-failure`
Expected: build succeeds with zero warnings; ctest reports `100% tests passed`. Test count increases by 3.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Camera2D.h tests/test_camera2d.cpp
git commit -m "feat(gfx): add Camera2D adapter with fluent Follow/WithZoom/WithRotation"
```

Pre-commit guard: `git diff --cached -U0 | grep -Ei 'claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md'` must return nothing.

---

## Task 2: `Texture` move-only RAII wrapper

**Files:**
- Create: `include/gfx/Texture.h`

No unit test — `LoadTexture` requires a live OpenGL context (window). Smoke-tested via Task 5 main-loop wiring.

- [ ] **Step 1: Create `include/gfx/Texture.h`**

```cpp
#pragma once
#include "raylib.h"
#include <string>

namespace nccu::gfx {

// Move-only RAII handle for raylib's GPU texture. Two Texture objects
// must NEVER share a single ::Texture2D id, or the destructor will
// Unload twice and crash. Static Load() is the only public constructor.
class Texture {
public:
    static Texture Load(const std::string& path) {
        return Texture{::LoadTexture(path.c_str())};
    }

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& o) noexcept : tex_(o.tex_), owns_(o.owns_) {
        o.tex_ = {};
        o.owns_ = false;
    }
    Texture& operator=(Texture&& o) noexcept {
        if (this != &o) {
            if (owns_) ::UnloadTexture(tex_);
            tex_ = o.tex_;
            owns_ = o.owns_;
            o.tex_ = {};
            o.owns_ = false;
        }
        return *this;
    }

    ~Texture() { if (owns_) ::UnloadTexture(tex_); }

    int  Width()   const noexcept { return tex_.width;  }
    int  Height()  const noexcept { return tex_.height; }
    bool IsValid() const noexcept { return tex_.id != 0; }

    // Used by Renderer::Texture inside include/gfx/ only.
    const ::Texture2D& Raw() const noexcept { return tex_; }

private:
    explicit Texture(::Texture2D t) noexcept
        : tex_(t), owns_(t.id != 0) {}

    ::Texture2D tex_{};
    bool        owns_{false};
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build`
Expected: build succeeds, zero warnings. (Texture.h has no users yet — it just needs to compile clean.)

- [ ] **Step 3: Run tests to confirm baseline still green**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`.

- [ ] **Step 4: Commit**

```bash
git add include/gfx/Texture.h
git commit -m "feat(gfx): add move-only Texture RAII wrapper"
```

Pre-commit grep guard same as Task 1 Step 5.

---

## Task 3: `Renderer::Texture` draw helper

**Files:**
- Modify: `include/gfx/Renderer.h`

- [ ] **Step 1: Add `#include "gfx/Texture.h"` near the top of `Renderer.h`**

Edit `include/gfx/Renderer.h` — the includes block becomes:

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"
```

- [ ] **Step 2: Add the `Texture` member function before the closing brace**

Insert this method inside `class Renderer { … };`, after the existing `Pixel(...)` method, before the closing `};`:

```cpp
    // Method name `Texture` shadows the type nccu::gfx::Texture inside
    // the class scope, so we use the elaborated form `class Texture` to
    // refer to the type in the parameter list (mirrors the Rect pattern).
    Renderer& Texture(const class Texture& tex,
                      Vec2 pos,
                      Color tint = Colors::White) noexcept {
        ::DrawTextureV(tex.Raw(),
                       ::Vector2{pos.x, pos.y},
                       ::Color{tint.r, tint.g, tint.b, tint.a});
        return *this;
    }
```

- [ ] **Step 3: Build**

Run: `cmake --build build`
Expected: build succeeds with zero warnings.

- [ ] **Step 4: Run tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed` (no behavior change — pure addition).

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Renderer.h
git commit -m "feat(gfx): extend Renderer with Texture(tex, pos, tint) helper"
```

Pre-commit grep guard.

---

## Task 4: `CameraScope` RAII pairing `BeginMode2D` / `EndMode2D`

**Files:**
- Create: `include/gfx/CameraScope.h`

No unit test — needs a live raylib context (and is exercised by Task 5 smoke).

- [ ] **Step 1: Create `include/gfx/CameraScope.h`**

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Camera2D.h"

namespace nccu::gfx {

// RAII: BeginMode2D in the ctor, EndMode2D in the dtor. Must be created
// INSIDE a DrawScope and destroyed BEFORE the DrawScope ends — calling
// EndMode2D after EndDrawing is undefined behavior in raylib. Stack
// ordering with an inner `{}` block enforces this naturally.
//
// Non-copyable AND non-movable: a moved-from CameraScope would otherwise
// double-EndMode2D in its destructor.
class CameraScope {
public:
    explicit CameraScope(const Camera2D& cam) noexcept {
        ::BeginMode2D(::Camera2D{
            ::Vector2{cam.offset.x, cam.offset.y},
            ::Vector2{cam.target.x, cam.target.y},
            cam.rotation,
            cam.zoom
        });
    }
    ~CameraScope() { ::EndMode2D(); }

    CameraScope(const CameraScope&)            = delete;
    CameraScope& operator=(const CameraScope&) = delete;
    CameraScope(CameraScope&&)                 = delete;
    CameraScope& operator=(CameraScope&&)      = delete;
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build`
Expected: build succeeds with zero warnings.

- [ ] **Step 3: Run tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`.

- [ ] **Step 4: Commit**

```bash
git add include/gfx/CameraScope.h
git commit -m "feat(gfx): add CameraScope RAII around BeginMode2D/EndMode2D"
```

Pre-commit grep guard.

---

## Task 5: Wire worldmap + camera follow into `src/main.cpp`

**Files:**
- Modify: `src/main.cpp`

- [ ] **Step 1: Replace `src/main.cpp` with the wired version**

The current `src/main.cpp` (100 lines) draws everything in screen space. Replace its contents with:

```cpp
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"
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
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdio>

int main() {
    using namespace nccu::gfx;

    constexpr int kWinW = 800;
    constexpr int kWinH = 450;
    const Vec2    kScreenCenter{kWinW / 2.0f, kWinH / 2.0f};

    auto win = Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(kWinW, kWinH)
                   .Fps(60)
                   .Open();

    auto worldmap = Texture::Load("resources/assets/maps/worldmap.png");

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            Renderer{}.Rect(Rect{e.position.x, e.position.y, 20, 20}, e.color);
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{400, 225}));
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{150, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{300, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{500, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{650, 100}));

    Player* player = dynamic_cast<Player*>(objects.front().get());
    Camera2D cam;

    while (!win.ShouldClose()) {
        float dt = Time::DeltaSeconds();

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
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

        if (player) {
            cam.Follow(player->GetPosition(), kScreenCenter);
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

            // Screen-space HUD: stays pinned, does not scroll with the world.
            TextBuilder{"WASD: move    E: pick up"}
                .At(Vec2{10, 10}).Size(16).Color(Colors::DarkGray).Draw();
            if (player) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "karma: %d   umbrella: %s",
                    player->GetKarma(), player->HasUmbrella() ? "yes" : "no");
                TextBuilder{buf}
                    .At(Vec2{10, 30}).Size(16).Color(Colors::DarkGray).Draw();
            }
        }

        // End-of-frame sweep: deferred deletion avoids iterator invalidation.
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

Key differences from the pre-Phase-A `main.cpp`:
- Adds `auto worldmap = Texture::Load(...)` right after window opens.
- Adds `Camera2D cam` after `Player* player = ...`.
- Calls `cam.Follow(player->GetPosition(), kScreenCenter)` once per frame BEFORE entering `DrawScope`.
- Wraps the worldmap draw + per-object Draw() loop in `{ CameraScope view{cam}; … }`, scoped to die before HUD.
- HUD `TextBuilder` calls are outside the inner `{}` so they render in screen space.

- [ ] **Step 2: Build**

Run: `cmake --build build`
Expected: build succeeds with zero warnings under `-Wall -Wextra -Wpedantic`.

- [ ] **Step 3: Run tests**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`.

- [ ] **Step 4: Smoke-run the binary**

Run from worktree root:

```bash
./build/OOP_Raylib_Lab &
PID=$!
sleep 6
kill -0 "$PID" && echo OK || echo CRASHED
kill "$PID" 2>/dev/null
wait "$PID" 2>/dev/null
```

Expected: prints `OK` (process still alive after 6 seconds). If `CRASHED`, capture stderr and stop.

- [ ] **Step 5: Manual visual check (load-bearing)**

Launch interactively: `./build/OOP_Raylib_Lab`

Verify all four points by eye:
1. The worldmap (a 2048×2048 NCCU 山下 pixel-art tile) is visible filling more than the viewport, with the player rectangle and four coloured umbrella rectangles drawn on top.
2. Holding **W / A / S / D** moves the player AND scrolls the worldmap so the player stays near screen center (≈ 400, 225).
3. The HUD text ("WASD: move    E: pick up" and the karma/umbrella line) is fixed in the top-left corner and **does not move** when the worldmap scrolls.
4. Umbrellas stay anchored to their world positions (≈ y=100), so walking away from them leaves them behind on screen; walking back returns them to the same world spot.

If any of the four fails, stop and surface what is wrong.

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp
git commit -m "feat(game): load worldmap and follow player with Camera2D"
```

Pre-commit grep guard.

---

## Task 6: Verification gate + grep audit

**Files:** none (verification only)

- [ ] **Step 1: Clean rebuild from scratch**

```bash
rm -rf build
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build 2>&1 | tee /tmp/phaseA-build.log
```

Expected: build succeeds. Scan `/tmp/phaseA-build.log` for the strings `warning:` or `error:` in lines that reference any file inside `src/`, `tests/`, or `include/`. Raylib's own internal warnings (paths under `_deps/raylib-src/`) are ignored — they're not our project code.

Concretely: `grep -E '^(/.*/src/|/.*/include/|/.*/tests/).*: (warning|error):' /tmp/phaseA-build.log` must be empty.

- [ ] **Step 2: Full test suite**

Run: `ctest --test-dir build --output-on-failure`
Expected: `100% tests passed`. The total test-case count should be the prior baseline plus 3 (Camera2D defaults / Follow / WithZoom+WithRotation).

- [ ] **Step 3: Five-second smoke**

```bash
./build/OOP_Raylib_Lab &
PID=$!
sleep 6
kill -0 "$PID" && echo SURVIVED || echo CRASHED
kill "$PID" 2>/dev/null
wait "$PID" 2>/dev/null
```

Expected: `SURVIVED`.

- [ ] **Step 4: raylib.h self-containment grep**

Run: `grep -rn 'raylib.h' src/ tests/ include/ | grep -v 'include/gfx/'`
Expected: no output. If anything appears, stop and remove the leaked include.

- [ ] **Step 5: Architectural red-line greps**

```bash
# Player must not include any concrete TransparentUmbrella subclass header.
grep -nE '#include\s+"(TrueUmbrella|FragileUmbrella|ProfessorTrapUmbrella|CursedUmbrella)\.h"' include/Player.h src/Player.cpp
# Expected: no output.

# Item / TransparentUmbrella family must not call DrawText or DrawTexture.
grep -nE 'Draw(Text|Texture)' include/Item.h include/TransparentUmbrella.h src/TrueUmbrella.cpp src/FragileUmbrella.cpp src/ProfessorTrapUmbrella.cpp src/CursedUmbrella.cpp 2>/dev/null
# Expected: no output.

# Main loop must not call `.erase(` or `delete` from inside the per-object iteration loop.
# (Phase A only adds CameraScope / camera follow; deferred-sweep block is unchanged.)
grep -n 'objects.erase' src/main.cpp
# Expected: exactly one match, located AFTER the closing brace of the DrawScope block.
```

If any of the three greps above produces unexpected output, stop and fix before merging.

- [ ] **Step 6: External-tool / agent reference grep on the full diff vs main**

```bash
git diff main -U0 -- ':(exclude)docs/superpowers' | \
    grep -Ei 'claude|codex|claude-code|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md'
```

Expected: no output. (`docs/superpowers/` is gitignored anyway, but the exclude is belt-and-braces.)

- [ ] **Step 7: Finish the development branch**

Once all six checks pass, finish via `superpowers:finishing-a-development-branch`. Defaults for this project (per repo memory):
- Do **not** auto-push.
- Squash-merge into `main` is the preferred option (matches the wrapper-layer merge from 2026-05-11).
- The user runs `git push origin main` themselves afterward.

---

## Manual visual checklist (sticky reference)

When you launch `./build/OOP_Raylib_Lab` in Task 5 Step 5, you should see:

| What | Expected |
|---|---|
| Background | Pre-generated worldmap PNG visible across the viewport, much wider than 800×450 |
| Player rectangle | Stays near screen center as you move |
| Umbrellas | Anchored to fixed world positions; scroll past as the player walks |
| HUD ("WASD: move…", "karma: 0 …") | Pinned to top-left, never scrolls |
| Walking past worldmap edge (no Phase B clamp yet) | Background turns to RayWhite outside the texture — that's expected; Phase B will clamp |

---

## Self-Review Notes (from plan author)

Coverage vs spec:
- Spec §2 goals — all five items covered: Texture (Task 2), Camera2D (Task 1), CameraScope (Task 4), Renderer::Texture (Task 3), worldmap load + follow + HUD-outside (Task 5).
- Spec §5 pattern table — RAII / Adapter / Fluent / Builder-via-factory all present.
- Spec §7 test plan — `tests/test_camera2d.cpp` with Follow + Zoom-equivalent + defaults all present in Task 1.
- Spec §8 verification gate — every clause greppable in Task 6.

Intentional deviations from spec:
- `Camera2D` fluent zoom/rotation setters are `WithZoom` / `WithRotation` instead of `Zoom` / `Rotation`, to avoid a member-name collision with the public fields `zoom` and `rotation`. Spec §5 sketch used `Zoom(1.0f)`. Test cases assert via the public fields, which is what the spec actually requires in §7.
- `Camera2D.h` does not include `raylib.h` — the raylib conversion happens at the `CameraScope` boundary instead, keeping `Camera2D` a pure-data adapter that can be used in unit tests without any raylib context. Spec §5 said "an internal `operator ::Camera2D() const` is safe" — we chose explicit brace-init at the boundary for the same reason `Color.h` / `Vec2.h` / `Rect.h` avoid implicit conversions: it eliminates ambiguous-overload risks.

Type consistency check:
- `Texture::Load(path)` → `Texture` value (move-only). Used by `auto worldmap = Texture::Load(...)` in Task 5. ✓
- `Renderer::Texture(const Texture&, Vec2, Color)` signature in Task 3 matches the call site `Renderer{}.Texture(worldmap, Vec2{0,0})` in Task 5. ✓
- `Camera2D::Follow(Vec2, Vec2) -> Camera2D&` signature in Task 1 matches the call `cam.Follow(player->GetPosition(), kScreenCenter)` in Task 5. ✓
- `CameraScope(const Camera2D&)` ctor in Task 4 matches `CameraScope view{cam}` in Task 5. ✓
