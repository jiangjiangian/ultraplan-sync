# Audit — docs/superpowers/specs/2026-05-11-camera-worldmap-design.md

**Overview (≤3):**
- Phase A scope (Texture/Camera2D/CameraScope/Renderer::Texture + worldmap follow + HUD outside camera) is fully shipped in `include/gfx/*` and `src/View.cpp`. Spec says construction happens in `main.cpp`; in shipped code the wiring lives one level deeper in `nccu::View` (`include/View.h`, `src/View.cpp`) — stale-doc-only.
- Spec is Phase A-only and explicitly excludes boundary clamp (Phase B); shipped code already implements `Camera2D::ClampToWorld` and calls it every frame, exceeding the spec — stale-doc-only.
- Asset name drift: spec loads `resources/assets/maps/worldmap.png`; shipped View loads `worldmap_base.png` (terrain-only base for walk-behind buildings) — stale-doc-only.

## Per-element annotations

- **§2 Goal — `nccu::gfx::Texture` RAII wrapper (Load/Unload)**
  - **[是否實作?]** Yes — `include/gfx/Texture.h:11-50`
  - **[邏輯衝突?]** No

- **§2 Goal — `nccu::gfx::Camera2D` adapter with `Follow(target, screenCenter)`**
  - **[是否實作?]** Yes — `include/gfx/Camera2D.h:10-22`
  - **[邏輯衝突?]** No

- **§2 Goal — `nccu::gfx::CameraScope` RAII (BeginMode2D/EndMode2D)**
  - **[是否實作?]** Yes — `include/gfx/CameraScope.h:15-31`
  - **[邏輯衝突?]** No

- **§2 Goal — `Renderer::Texture(...)` draw helper**
  - **[是否實作?]** Yes — `include/gfx/Renderer.h:42-49`
  - **[邏輯衝突?]** No

- **§2 Goal — Load worldmap PNG at startup and draw as background**
  - **[是否實作?]** Yes — `src/View.cpp:32, 77` (loads `worldmap_base.png`, drawn at world origin)
  - **[邏輯衝突?]** Yes — Stale-doc-only. Spec names `worldmap.png`; shipped asset is `worldmap_base.png` (terrain-only base; building sprites are now separate textures for walk-behind, see View.cpp:26-30).

- **§2 Goal — Single CameraScope wrapping player/umbrella render**
  - **[是否實作?]** Yes — `src/View.cpp:75-112`
  - **[邏輯衝突?]** No (extended: also wraps building sprites + painter's-order sort)

- **§2 Goal — HUD drawn OUTSIDE CameraScope (screen-pinned)**
  - **[是否實作?]** Yes — `src/View.cpp:119-380` (all HUD/dialog/menu draws live after the `{ CameraScope … }` block closes at line 112)
  - **[邏輯衝突?]** No

- **§3 Non-Goal — Boundary clamping (Phase B)**
  - **[是否實作?]** N/A (Phase A) — but shipped: `Camera2D::ClampToWorld` (`include/gfx/Camera2D.h:27-37`) and called in View (`src/View.cpp:70-71`)
  - **[邏輯衝突?]** Yes — Stale-doc-only. Phase A spec defers clamp to Phase B; shipped code already includes it. Tests cover it (`tests/test_camera2d_clamp.cpp`).

- **§3 Non-Goal — `TextureRegion` / sprite atlas placeholder API**
  - **[是否實作?]** Yes (placeholder exercised) — `Renderer::TextureRect` in `include/gfx/Renderer.h:55-65`; `IRenderer::DrawSprite` in `include/gfx/IRenderer.h:21`; building flip uses negative source extents (`src/View.cpp:102-109`).
  - **[邏輯衝突?]** No (spec said "placeholder API but not exercised"; shipped exercises it for building flip + character select, but spec called this out explicitly).

- **§4 Architecture — `raylib.h` confined to `include/gfx/`**
  - **[是否實作?]** Partial — also leaks into `src/Harness.cpp:12` and `src/ScriptInput.cpp:10` (autoplay harness)
  - **[邏輯衝突?]** No — Stale-doc-only. Harness/ScriptInput are post-Phase-A additions (CLAUDE.md §4) governed by separate rules.

- **§5 Pattern — `Texture` move-only, owns_ guarded UnloadTexture**
  - **[是否實作?]** Yes — `include/gfx/Texture.h:17-35,45-49`
  - **[邏輯衝突?]** No

- **§5 Pattern — `Camera2D` fluent `.Follow(...).Zoom(z)` chaining**
  - **[是否實作?]** Yes — but the zoom setter is named `WithZoom`, not `Zoom` (`include/gfx/Camera2D.h:21`)
  - **[邏輯衝突?]** Yes — Stale-doc-only. Naming drift only; behaviour matches.

- **§5 Pattern — `CameraScope` non-copyable AND non-movable**
  - **[是否實作?]** Yes — `include/gfx/CameraScope.h:27-30`
  - **[邏輯衝突?]** No

- **§5 Pattern — `Camera2D` layout matches raylib's `::Camera2D` (internal conversion safe)**
  - **[是否實作?]** Yes — fields match (`include/gfx/Camera2D.h:11-14`); conversion done explicitly in `CameraScope` ctor (`include/gfx/CameraScope.h:18-23`) rather than via `operator ::Camera2D() const`
  - **[邏輯衝突?]** Yes — Stale-doc-only. Spec mentions `operator ::Camera2D() const`; shipped uses brace-init at the call site. Semantics identical.

- **§6 Touch Point — `Renderer.h` add `Texture(...)` overload**
  - **[是否實作?]** Yes — `include/gfx/Renderer.h:42-49`
  - **[邏輯衝突?]** No

- **§6 Touch Point — `src/main.cpp` constructs Texture + Camera2D, wraps draw in CameraScope, HUD outside**
  - **[是否實作?]** Yes (relocated) — `src/main.cpp:78` constructs `nccu::View`; all texture/camera wiring moved into `View::View` and `View::Draw` (`src/View.cpp:31-54, 56-112`)
  - **[邏輯衝突?]** Yes — Stale-doc-only. Refactor put the wiring in `View` (cleaner MVC); `main.cpp` is now the composition root as CLAUDE.md §5 demands. Behaviour matches.

- **§7 Test — `tests/test_camera2d.cpp` (Follow/Zoom/default zoom 1.0)**
  - **[是否實作?]** Yes — `tests/test_camera2d.cpp` (3 cases: defaults, Follow, WithZoom+WithRotation)
  - **[邏輯衝突?]** No

- **§8 Verification Gate items 1-6** — gating contract; not a code element, see CLAUDE.md §7.
  - **[是否實作?]** N/A (process)
  - **[邏輯衝突?]** No

- **§10 Risk — `Texture` move-only, source nulled on move**
  - **[是否實作?]** Yes — `include/gfx/Texture.h:20-33` (move ctor + assign null `tex_`/`owns_`)
  - **[邏輯衝突?]** No

- **§10 Risk — `CameraScope` inside DrawScope only**
  - **[是否實作?]** Yes — `src/View.cpp:75-112` (inner `{}` block inside `Draw()` which is called between BeginDrawing/EndDrawing in `main.cpp`)
  - **[邏輯衝突?]** No

- **§10 Risk — `Renderer::Texture` argument outlives the draw call**
  - **[是否實作?]** Yes — `worldmap_` is a `View` member (`include/View.h:49`), lives as long as `View`.
  - **[邏輯衝突?]** No

- **§10 Risk — Worldmap 2048×2048; cam.target=playerPos, no clamp Phase A**
  - **[是否實作?]** Partial — size matches (`include/WorldConfig.h:8`, `kSize=2048.0f`); but ClampToWorld IS applied (`src/View.cpp:70-71`).
  - **[邏輯衝突?]** Yes — Stale-doc-only (see Phase B non-goal divergence above).

## Summary

- Elements audited: **22**
- Yes: **17** · Partial: **2** · No: **0** · N/A: **3**
- Conflicts: **6** — all **Stale-doc-only** (no Real conflicts)
  - `worldmap.png` → `worldmap_base.png` rename
  - Phase B clamp already shipped in Phase A
  - `Zoom(z)` named `WithZoom(z)`
  - `operator ::Camera2D() const` realised as explicit brace-init at use site
  - main.cpp wiring relocated into `View` (better MVC)
  - `raylib.h` also in Harness/ScriptInput (separate post-Phase-A subsystem)
- Real conflicts: **0**. Spec is fully realised and extended beyond Phase A; documentation needs a refresh, the code does not.
