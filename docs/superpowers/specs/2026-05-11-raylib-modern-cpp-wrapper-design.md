# Raylib Modern C++ Wrapper — Design Spec

**Date:** 2026-05-11
**Project:** 《尋傘記：政大山下篇》(`/Users/ian/Desktop/assignment-5-jiangjiangian`)
**Status:** Approved (brainstorming complete; ready for planning)

---

## 1. Context

The current codebase calls raylib C functions directly from `main.cpp`, `Player.cpp`, and EventBus subscribers (e.g. `InitWindow`, `BeginDrawing`, `DrawRectangle`, `IsKeyDown`, `GetFrameTime`). This works but does not showcase modern C++ idioms or OO design patterns required by the OOP final project.

This spec defines a header-only C++17 wrapper layer that hides raylib behind a fluent, RAII-based, type-safe API and migrates every existing direct raylib call to use it.

## 2. Goals

- Hide every direct `raylib.h` include behind the wrapper layer; game code touches only `nccu::gfx::*`
- Showcase: **RAII**, **Builder**, **Fluent / Method Chaining**, **Facade**, **Adapter**
- Zero runtime overhead — all wrappers `inline` / `constexpr`-friendly so the compiler folds them away
- Existing GoF patterns (Factory Method, Observer, Template Method) and game logic remain untouched

## 3. Non-Goals

- Wrap audio (`raudio`), 3D models (`rmodels`), or anything the game does not currently use
- Replace EventBus, GameObjectFactory, or the umbrella hierarchy
- Introduce a Singleton

## 4. Architecture

Header-only library under `include/gfx/`, namespace `nccu::gfx`.

```
include/gfx/
├── Color.h         Adapter — wraps raylib::Color, .WithAlpha(f) fluent
├── Vec2.h          Adapter — operator+ - * scalar, length
├── Rect.h          Adapter — Contains(point), Intersects(rect)
├── Key.h           enum class Key — type-safe wrap of raylib KEY_* constants
├── Window.h        RAII + Builder
├── DrawScope.h     RAII for BeginDrawing / EndDrawing
├── Renderer.h      Fluent free-standing helper for one-shot draws
├── TextBuilder.h   Builder for text with chained .At/.Size/.Color/.Draw
├── Input.h         Facade — IsDown / IsPressed / IsReleased
└── Time.h          Facade — DeltaSeconds / FpsAvg
```

**Self-containment rule:** `#include "raylib.h"` appears **only** inside files under `include/gfx/`. Game code (`src/*.cpp` outside the wrapper, `tests/*.cpp`, all non-gfx headers) must not reference `raylib.h` directly. `GameObject`, `Character`, `Item`, `Player`, etc. switch their stored types from raylib's `Vector2` / `Rectangle` / `Color` to `nccu::gfx::Vec2` / `Rect` / `Color`.

`Vec2`, `Rect`, `Color` are **wrapper structs** with the same memory layout as their raylib counterparts. Internal (i.e. inside `nccu::gfx::*`) conversion to / from raylib types is allowed via `operator raylib::Vector2() const` and friends; these conversion operators are **not** exposed to game code (game code never sees raylib types).

A predefined palette `nccu::gfx::Colors` provides constexpr instances mirroring raylib's well-known colors (RAYWHITE, BLUE, BLACK, WHITE, DARKGRAY, etc.).

## 5. Components & Pattern Map

| Class / Free helper | Pattern | Public API sketch |
|---|---|---|
| `Window` | RAII + Builder | `auto win = Window::Builder().Title("…").Size(800,450).Fps(60).Open();` — `~Window()` calls `CloseWindow`; move-only, non-copyable |
| `DrawScope` | RAII | `{ DrawScope frame; /* draws */ }` — ctor `BeginDrawing`, dtor `EndDrawing`; non-copyable, non-movable |
| `Renderer` | Fluent | `Renderer{}.Clear(Colors::RayWhite).Rect({x,y,w,h}, Colors::Blue).Pixel({x,y}, c);` returning `Renderer&` |
| `TextBuilder` | Builder | `TextBuilder{"karma: 50"}.At({10,30}).Size(16).Color(Colors::DarkGray).Draw();` |
| `Input` | Facade + type-safe | `Input::IsDown(Key::W)`; `Key` is `enum class` mapping to raylib KEY_* |
| `Color` | Adapter + Fluent | `constexpr Color{r,g,b,a}`; `.WithAlpha(uint8_t)`; implicit conversion to raylib::Color is internal-only |
| `Vec2` | Adapter | `constexpr` arithmetic |
| `Rect` | Adapter | `Contains(Vec2)`, `Intersects(Rect)` |
| `Time` | Facade | `Time::DeltaSeconds()`, `Time::FpsAvg()` |

Existing GoF patterns (unchanged):

| Pattern | Where |
|---|---|
| Factory Method | `GameObjectFactory::Create` |
| Observer | `EventBus::Subscribe` / `Publish` |
| Template Method | `TransparentUmbrella::beClaimed` + 4 subclasses |

## 6. Touch Points (existing files that change)

| File | Change |
|---|---|
| `src/main.cpp` | Use `Window::Builder` + `DrawScope` + `Renderer` + `TextBuilder` + `Input::IsPressed(Key::E)` + `Time::DeltaSeconds()` |
| `src/Player.cpp` | `Draw()` uses `Renderer{}.Rect(hitBox_, Colors::Blue)`; `HandleInput()` uses `Input::IsDown(Key::W/A/S/D)` |
| `src/TransparentUmbrella.cpp` | Unchanged (still publishes `RenderRequested` event) |
| 4 umbrella concrete `.cpp` | Unchanged |
| RenderRequested subscriber in `main.cpp` | Replaces `DrawRectangle` with `Renderer{}.Rect(...)` |
| `include/GameObject.h`, `Character.h`, `Item.h`, `Player.h`, `EventBus.h`, `Transparent*.h` etc. | Replace `#include "raylib.h"` with the relevant `gfx/*.h` headers; switch stored field types from `Vector2`→`Vec2`, `Rectangle`→`Rect`, `Color`→`Color` (now `nccu::gfx::*`). Method signatures like `Move(Vector2 dir, float dt)` change to `Move(Vec2 dir, float dt)` accordingly |

## 7. Tests

Add doctest cases that exercise pure-data wrappers:

- `tests/test_color.cpp` — construction, equality, `WithAlpha` chain, `constexpr` usability
- `tests/test_rect.cpp` — `Contains`, `Intersects`, edge cases (empty rect, point on edge)
- `tests/test_vec2.cpp` — arithmetic, length, `constexpr` correctness
- `tests/test_text_builder.cpp` — chained setters preserve state, `Draw()` is a no-op until `DrawScope` is active

Wrappers that depend on raylib global state (Window, DrawScope, Renderer, Input, Time) are not unit-tested; they are validated via the smoke run.

## 8. Verification Gate

Before merging the worktree back to main:

1. `cmake --build build` succeeds with `-Wall -Wextra -Wpedantic` and **0 warnings**
2. `ctest` reports all cases passing (existing 9 + new ≥ 4)
3. `./build/OOP_Raylib_Lab` launches and survives ≥ 5 seconds without crashing; visual output matches pre-refactor (player blue square, four umbrellas in a row, HUD text, E-key pickup, karma display)
4. `grep -rn "raylib.h" src/ tests/ include/ --include="*.cpp" --include="*.h" | grep -v "include/gfx/"` returns no matches

## 9. Implementation Order Suggestion (for planner)

The wrappers form a layered dependency chain — implement bottom-up:

1. `Color.h` + `Vec2.h` + `Rect.h` + `Key.h` (pure data adapters; no raylib state)
2. `Time.h` + `Input.h` (stateless facades over raylib free functions)
3. `Window.h` + `DrawScope.h` (RAII)
4. `Renderer.h` + `TextBuilder.h` (fluent draw helpers; depend on Color/Vec2/Rect)
5. New tests (color/rect/vec2/text-builder)
6. Migrate `Player.cpp` (small surface)
7. Migrate `main.cpp` (largest surface)
8. Verification gate

Each step keeps the codebase compiling and tests green before the next.

## 10. Risks & Invariants

- **`DrawScope` lifetime:** must be created after `Window` is open and destroyed before `Window` closes. Misuse → raylib UB. Document in `DrawScope.h` and rely on RAII to make incorrect usage hard.
- **`Window` move-only:** copying would call `CloseWindow` twice. Delete copy ctor / copy assign; default move with proper invariant ("moved-from window does not call CloseWindow on destruction").
- **No `Singleton`:** `Renderer{}` is a stateless temporary value; chained calls are inlined. `Input` and `Time` expose only `static` member functions, not Singleton instances.
- **Implicit conversions:** internal-only between wrapper types and raylib types. Do **not** make conversion operators public — that would leak raylib types into game code.
- **EventBus payload (`Event::position`, `Event::color`):** currently typed as `Vector2` / `Color` (raylib). The plan must migrate these to `nccu::gfx::Vec2` / `nccu::gfx::Color` to keep `EventBus.h` raylib-free (consistent with the self-containment rule). Subscribers in `main.cpp` then receive wrapper types directly.

## 11. Out-of-Scope (parking lot)

- Texture / Font / Sound RAII — not yet used by the game; add when sprites land
- Particle / shader helpers — Day 3+ work
- Mouse / gamepad input — keyboard-only for now
