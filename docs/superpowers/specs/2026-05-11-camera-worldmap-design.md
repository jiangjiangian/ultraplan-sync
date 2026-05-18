# Phase A — Camera + Texture + Worldmap Follow

**Date:** 2026-05-11
**Project:** 《尋傘記：政大山下篇》(`/Users/ian/Desktop/assignment-5-jiangjiangian`)
**Status:** Approved (brainstorming complete; ready for planning)
**Parent design:** `2026-05-11-raylib-modern-cpp-wrapper-design.md` (the nccu::gfx wrapper layer this builds on)

---

## 1. Context

Following the squash-merge of the `nccu::gfx` wrapper layer into main (commit `1744152`), the game still draws to a fixed 800×450 viewport with no world map underneath. Player coordinates are world-space already, but no camera transform is applied, so movement past the window edge leaves the visible area. This phase introduces the camera + texture infrastructure so the player can roam the pre-generated 2048×2048 NCCU 山下 worldmap with the camera following along.

This is **Phase A** of a three-phase sequence (A: camera+worldmap follow → B: boundary clamp → C: 27 building entry triggers). Phases B and C have their own future specs.

## 2. Goals

- Add a `nccu::gfx::Texture` RAII wrapper covering `LoadTexture` / `UnloadTexture`
- Add a `nccu::gfx::Camera2D` adapter struct with a fluent `Follow(target, screenCenter)` helper
- Add a `nccu::gfx::CameraScope` RAII pairing `BeginMode2D` / `EndMode2D`
- Extend `nccu::gfx::Renderer` with a `Texture(...)` draw helper
- Load `resources/assets/maps/worldmap.png` at startup, draw it as the world background
- Run a single CameraScope inside the main draw frame so player + umbrellas render in world space
- Keep HUD (operation hint + karma line) rendering **outside** the CameraScope so it stays screen-pinned

## 3. Non-Goals

- World boundary clamping (Phase B)
- Building entry triggers (Phase C)
- Mouse / scroll wheel camera control
- Camera zoom keybindings
- Multiple cameras / minimaps
- Sprite atlas, animation, or `TextureRegion` (rectangular sub-sampling) — covered by a placeholder API but not exercised this phase
- Player / NPC / umbrella sprite art (no usable sprite assets yet; existing rectangles remain)
- Audio

## 4. Architecture

Header-only additions under `include/gfx/`, namespace `nccu::gfx`:

```
include/gfx/
├── Texture.h        New — RAII handle for raylib::Texture2D
├── Camera2D.h       New — adapter struct; Follow(target, screenCenter) helper
├── CameraScope.h    New — RAII pairing BeginMode2D / EndMode2D
└── Renderer.h       Modified — adds Texture(const Texture&, Vec2 pos, Color tint = white)
```

**Self-containment rule** carries over from Phase 1: `raylib.h` only inside `include/gfx/`. Game code (`src/main.cpp` etc.) keeps interacting with wrappers only.

## 5. Components & Pattern Map

| Class / Helper | Pattern | Public API sketch |
|---|---|---|
| `Texture` | **RAII + Builder (static factory) + move-only** | `auto t = Texture::Load("path.png");` — `~Texture()` calls `UnloadTexture` iff `owns_`; non-copyable; move ctor / assign transfer ownership and null the source |
| `Camera2D` | **Adapter + Fluent** | `Camera2D c; c.Follow(playerPos, Vec2{400,225}).Zoom(1.0f);` returns `Camera2D&` |
| `CameraScope` | **RAII** | `{ CameraScope view{cam}; … }` — ctor `BeginMode2D(cam)`, dtor `EndMode2D()`; non-copyable, non-movable |
| `Renderer::Texture` | **Fluent** | `Renderer{}.Texture(t, Vec2{0,0});` returns `Renderer&` |

`Camera2D` (wrapper struct) layout matches raylib's `::Camera2D` so an internal `operator ::Camera2D() const` is safe and is only consumed inside `include/gfx/`.

## 6. Touch Points (existing files)

| File | Change |
|---|---|
| `include/gfx/Renderer.h` | Add `Texture(const Texture&, Vec2, Color = Colors::White)` |
| `src/main.cpp` | Construct `Texture` from `resources/assets/maps/worldmap.png` after `Window::Builder().Open()`. Construct `Camera2D`, call `cam.Follow(player->GetPosition(), Vec2{winW/2, winH/2})` per frame. Wrap the world draws (worldmap + GameObjects) in `CameraScope`. Keep `TextBuilder` HUD calls outside the CameraScope. |
| `CMakeLists.txt` | No change |

The `Texture` ctor takes a path; it `LoadTexture`s and reports failure via raylib's existing log (a failed load returns a zero-id texture and raylib logs `WARNING: TEXTURE: …`). For Phase A this is acceptable; Phase B/C may add an `IsValid()` check.

## 7. Tests

Wrapper layer additions in this phase are mostly raylib-state-bound (Texture loads GPU resources, Camera/CameraScope drive raylib globals). They are not unit-tested directly; smoke run is the verification.

Add one pure-data unit test set:

- `tests/test_camera2d.cpp` — `Camera2D::Follow(target, screenCenter)` should set `target = target` and `offset = screenCenter`. `Zoom(z)` should set `zoom = z`. Default zoom is 1.0.

That's the only new test file expected.

## 8. Verification Gate

Before merging the Phase A worktree back to main:

1. `cmake --build build` zero project-code warnings under `-Wall -Wextra -Wpedantic`
2. `ctest` all green (existing 32 + new ≥ 3 from test_camera2d)
3. `./build/OOP_Raylib_Lab` survives ≥ 5 seconds
4. **Manual visual check** (this is the load-bearing one — phase A is mostly visual):
   - Worldmap is visible underneath player + umbrellas
   - Pressing W/A/S/D moves the player AND scrolls the worldmap so the player stays near screen center
   - HUD text ("WASD: move    E: pick up" and karma) is fixed on screen (does NOT scroll with the world)
   - Umbrella rectangles stay anchored to their original world positions, not the screen
5. `grep -rn "raylib.h" src/ tests/ include/ | grep -v "include/gfx/"` still empty
6. Naming-hygiene grep still clean (`Claude|Codex|claude-code|superpowers|nanobanana|Gemini|Anthropic` absent from tracked content except `.gitignore` path patterns)

## 9. Implementation Order Suggestion (for planner)

Same TDD layered bottom-up rhythm as the previous wrapper layer:

1. `Camera2D.h` (pure adapter + tests for Follow/Zoom)
2. `Texture.h` (RAII; smoke-only)
3. `Renderer.h` — add Texture method
4. `CameraScope.h` (RAII; smoke-only)
5. `src/main.cpp` — wire the four pieces together, load worldmap, follow player

Each step keeps the project compiling and tests green before the next.

## 10. Risks & Invariants

- **`Texture` ownership:** must be move-only. Two textures sharing one `::Texture2D` id would double-`UnloadTexture` and crash. Move ctor / assign nulls the source's id. Document in the header.
- **`CameraScope` lifetime:** must be created *inside* a `DrawScope` and destroyed before the `DrawScope` ends. raylib's `EndMode2D` after `EndDrawing` is UB. Rely on stack ordering — the spec example wraps CameraScope in an inner `{}` block before DrawScope's natural end.
- **`Renderer::Texture` invariant:** the `Texture` argument must outlive the draw call. Since Phase A constructs the worldmap texture at startup and uses it every frame, lifetime is satisfied by `main`'s stack.
- **Asset path resolution:** raylib looks up `resources/...` relative to the working directory. CMake copies `resources/` next to the executable, so launching from `./build/OOP_Raylib_Lab` works. Launching from the repo root with `./build/OOP_Raylib_Lab` also works (resources both at `./resources` and `./build/resources`). Do not absolutize.
- **HUD outside CameraScope:** if HUD is rendered inside the camera transform, text will be offset by the camera's `target - offset` vector and may even disappear off screen. The plan explicitly closes the CameraScope before HUD calls.
- **Worldmap size vs viewport:** 2048×2048 world > 800×450 viewport. With `cam.target = playerPos`, `cam.offset = Vec2{winW/2, winH/2}`, the player stays at screen center. Phase A does NOT clamp `cam.target`, so walking past the worldmap edge reveals empty background — that's Phase B's job.

## 11. Out-of-Scope (parking lot)

- Boundary clamp (Phase B) — needs the worldmap dimensions plus optional walkable-region mask
- Building entry triggers (Phase C) — needs a building registry, position list, and event integration
- `TextureRegion` / sprite atlas — when sprite assets land
- Camera shake / zoom animation
- Pixel-art nearest-neighbour filtering toggle (raylib default bilinear may blur the pixel art; add later if visually unacceptable)
