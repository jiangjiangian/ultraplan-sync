# Audit — docs/superpowers/plans/2026-05-11-camera-worldmap.md

**Overview (≤3):**
- Phase-A camera/worldmap plan (5 new files, 2 modified) is fully **Yes** — `Camera2D`, `Texture`, `CameraScope`, `Renderer::Texture`, test cases, and worldmap-load-with-camera-follow all shipped. The integration moved into `View.cpp` (not `main.cpp` as the plan literally drafted) which is a stricter MVC outcome — the camera lives with rendering instead of the composition root, and `main.cpp` (HEAD `b33db2b`, 120 lines) is a thin Title/Select/Playing loop with no `gfx::Camera2D` reference.
- One feature was added beyond the plan (`Camera2D::ClampToWorld` with 5 dedicated tests in `tests/test_camera2d_clamp.cpp`) plus `Renderer::TextureRect` for sprite-sheet slicing. Both are pure additions, no plan-vs-shipped logic conflict.
- Two **stale-doc-only** divergences: the plan's `src/main.cpp` skeleton differs from the real `main.cpp` (the camera moved to `View`, and Title/Select were added by Cycle 6 UI work — superseded), and the raylib.h self-containment grep would fail today (`src/Harness.cpp:12`, `src/ScriptInput.cpp:10` include `raylib.h`) — but those are deliberate harness exceptions documented in CHANGELOG (B5/scripts), unrelated to Phase A.

## Per-element annotations

- **Task 0 — Baseline build/test in fresh worktree** — 4 steps (configure, build, ctest, branch check).
  **[是否實作?]** N/A — procedural; the project builds clean per CHANGELOG Cycle-8 gate.
  **[邏輯衝突?]** No.

- **Task 1 — `include/gfx/Camera2D.h` adapter struct (offset/target/rotation/zoom + Follow/WithZoom/WithRotation)** — pure-data adapter, no `raylib.h`.
  **[是否實作?]** Yes — `include/gfx/Camera2D.h:10-22` matches plan verbatim (struct + fluent setters + correct defaults).
  **[邏輯衝突?]** No.

- **Task 1 — `tests/test_camera2d.cpp` (defaults / Follow / WithZoom+WithRotation, 3 cases)**.
  **[是否實作?]** Yes — `tests/test_camera2d.cpp:7-33` is byte-equivalent to the plan's draft (same 3 TEST_CASE names and assertions).
  **[邏輯衝突?]** No.

- **Task 2 — `include/gfx/Texture.h` move-only RAII (`Load`/move-ctor/move-assign/dtor/`Width`/`Height`/`IsValid`/`Raw`)**.
  **[是否實作?]** Yes — `include/gfx/Texture.h:11-50` matches the plan exactly (delete copy, move with owns_ flip, Unload on owned dtor).
  **[邏輯衝突?]** No.

- **Task 3 — `Renderer::Texture(const class Texture&, Vec2, Color)` draw helper + `#include "gfx/Texture.h"`**.
  **[是否實作?]** Yes — `include/gfx/Renderer.h:6` includes Texture.h; `include/gfx/Renderer.h:42-49` implements the helper exactly as drafted (DrawTextureV, elaborated `class Texture`).
  **[邏輯衝突?]** No.

- **Task 4 — `include/gfx/CameraScope.h` RAII (`BeginMode2D`/`EndMode2D`, non-copy non-move)**.
  **[是否實作?]** Yes — `include/gfx/CameraScope.h:15-31` is the plan draft verbatim (raylib brace-init, four deleted special members).
  **[邏輯衝突?]** No.

- **Task 5 — Modify `src/main.cpp`: load worldmap, construct `Camera2D cam`, call `cam.Follow(player.pos, screenCenter)` per frame, wrap world draws in `{ CameraScope view{cam}; ... }`, keep HUD outside**.
  **[是否實作?]** Yes — but relocated. The camera/worldmap/CameraScope live in `src/View.cpp:32` (worldmap_ field), `:70-72` (Follow + ClampToWorld), `:76-77` (CameraScope cam + worldmap blit), `:74` (Clear before CameraScope), and HUD at `:114-` is outside the scope. `src/main.cpp:28-120` (HEAD `b33db2b`) is the composition root with `nccu::View view{kWinW, kWinH}` and never touches `gfx::Camera2D` directly.
  **[邏輯衝突?]** Stale-doc-only — the plan literally edited main.cpp because at plan-writing time `View` did not exist as the rendering owner. Real outcome (camera owned by View) is the better MVC fit (CLAUDE.md §5 "View = render only" / "main.cpp is a thin composition root"). All five behavioural points the plan lists (worldmap visible, player centred, HUD pinned, umbrellas world-anchored, world clear when off-map) hold via View instead.

- **Task 5 — `auto worldmap = Texture::Load("resources/assets/maps/worldmap.png")`**.
  **[是否實作?]** Yes — `src/View.cpp:32` loads `resources/assets/maps/worldmap_base.png` (the actual asset; `worldmap.png` also ships per `ls resources/assets/maps/`). Filename evolved to `worldmap_base.png` so terrain-only base composes with the per-building sprite overlays (`include/View.h:53` `buildingTextures_`).
  **[邏輯衝突?]** Stale-doc-only — plan named `worldmap.png`; current art pipeline uses `worldmap_base.png`. No behavioural conflict (it's still a 2048×2048 worldmap blit at origin).

- **Task 5 — HUD outside CameraScope (TextBuilder lines pinned)**.
  **[是否實作?]** Yes — `src/View.cpp:114-` HUD panel/text draws after the closing brace at `:112`. Camera-vignette block also gated on rain, outside scope.
  **[邏輯衝突?]** No.

- **Task 5 — End-of-frame deferred sweep of dead objects** (CLAUDE.md §5 red line).
  **[是否實作?]** Yes (relocated) — sweep lives in `src/GameController.cpp` (per Cycle-4/Cycle-6 ledger). `main.cpp` no longer carries the per-frame `objects.erase(...)` block the plan drafted.
  **[邏輯衝突?]** Stale-doc-only.

- **Task 6 Step 1 — Clean rebuild zero project warnings** under `-Wall -Wextra -Wpedantic`.
  **[是否實作?]** Yes — CHANGELOG Cycle-8 gate confirms "0 project warnings" (C++20).
  **[邏輯衝突?]** No.

- **Task 6 Step 2 — Full ctest** (baseline + 3 new Camera2D cases).
  **[是否實作?]** Yes, exceeded — CHANGELOG Cycle-8: 289/289 cases / 4254 assertions. The 3 plan cases shipped in `tests/test_camera2d.cpp` plus 5 extra in `tests/test_camera2d_clamp.cpp`.
  **[邏輯衝突?]** No (additive surplus).

- **Task 6 Step 3 — 5-second smoke survives**.
  **[是否實作?]** Yes — CLAUDE.md §7 gate (`playtest.sh` window ≥5 s no crash) is part of the cycle gate, last green at Cycle-8.
  **[邏輯衝突?]** No.

- **Task 6 Step 4 — raylib.h self-containment grep returns nothing outside `include/gfx/`**.
  **[是否實作?]** Partial — grep finds 2 hits: `src/Harness.cpp:12`, `src/ScriptInput.cpp:10` (sanctioned harness/scripted-input modules; not part of Phase A's surface).
  **[邏輯衝突?]** Stale-doc-only — the harness layer (CLAUDE.md §4) is allowed to touch `raylib.h` because it injects scripted input and screenshots; Phase A wrapper boundary holds for gameplay code. The plan's grep predates the harness landing.

- **Task 6 Step 5 — Architectural red-line greps** (Player→subclass include; Draw* in Item/umbrella; deferred sweep position).
  **[是否實作?]** Yes — invariants enforced project-wide (BUGLEDGER §5 "isActive_ idempotency"; CLAUDE.md §5 "Deferred deletion"). Sweep moved to `GameController` (still end-of-frame).
  **[邏輯衝突?]** No.

- **Task 6 Step 6 — No external-tool brand/model strings in committed code/diff** (CLAUDE.md §5 "Forbidden strings").
  **[是否實作?]** Yes — repo invariant; lint enforced before each commit per cycle ledger.
  **[邏輯衝突?]** No.

- **Task 6 Step 7 — Finish branch via superpowers:finishing-a-development-branch, squash-merge to main, no auto-push**.
  **[是否實作?]** N/A — workflow guidance; lead-merged per CHANGELOG.
  **[邏輯衝突?]** No.

- **Project Invariant 1 — wrapper self-containment** (raylib.h only in `include/gfx/`).
  **[是否實作?]** Partial — same exception as Task 6 Step 4 (Harness.cpp, ScriptInput.cpp).
  **[邏輯衝突?]** Stale-doc-only.

- **Project Invariant 2 — no external-tool/agent references in committed content**.
  **[是否實作?]** Yes — repo invariant.
  **[邏輯衝突?]** No.

- **Project Invariant 3 — CMake configure uses `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`**.
  **[是否實作?]** Yes — CLAUDE.md §1 same flag; `6bf5a5e` baked the shim so a bare `cmake -B build` also works on CMake 4.x (additive).
  **[邏輯衝突?]** No.

- **Project Invariant 4 — Player.h no concrete umbrella includes; Item/umbrella no DrawText/DrawTexture; main loop no mid-iter erase**.
  **[是否實作?]** Yes — CLAUDE.md §5 ongoing red lines; verified each cycle.
  **[邏輯衝突?]** No.

- **Naming-note deviation — `WithZoom`/`WithRotation` (not `Zoom`/`Rotation`) to avoid field/method name collision; spec sketch's `Zoom(z)` overridden**.
  **[是否實作?]** Yes — `include/gfx/Camera2D.h:21-22`.
  **[邏輯衝突?]** No (intentional plan deviation, recorded in Self-Review Notes).

- **Self-Review type-consistency claims** (Texture::Load → value, Renderer::Texture sig, Camera2D::Follow sig, CameraScope ctor sig).
  **[是否實作?]** Yes — all four signatures match the shipped headers.
  **[邏輯衝突?]** No.

- **Manual visual checklist** (worldmap fills viewport; player centred; HUD pinned; umbrellas world-anchored; off-map = RayWhite).
  **[是否實作?]** Yes — confirmed via `src/View.cpp:74` (`Clear(Colors::RayWhite)` before the CameraScope) + `:70-72` (Follow+Clamp) + HUD outside scope. The off-map "RayWhite" symptom is now suppressed because `ClampToWorld` keeps the camera inside the worldmap — superior to the plan's "Phase B will clamp" note (already shipped).
  **[邏輯衝突?]** No (Phase-B clamp landed early as `Camera2D::ClampToWorld` — bonus, not a conflict).

- **Bonus: `Camera2D::ClampToWorld(worldSize, viewportSize)`** — Phase-B clamp baked into the Phase-A adapter.
  **[是否實作?]** Yes — `include/gfx/Camera2D.h:27-37` + `tests/test_camera2d_clamp.cpp` (5 cases).
  **[邏輯衝突?]** No (additive; plan footnote anticipated this for Phase B).

- **Bonus: `Renderer::TextureRect(tex, src, dest, tint)`** for sprite-sheet slicing / character-select previews.
  **[是否實作?]** Yes — `include/gfx/Renderer.h:55-65`.
  **[邏輯衝突?]** No (additive — used by buildings/character select; outside Phase-A scope).

## Summary

- Total elements audited: **27**
  - Yes: 23 (incl. 2 bonus extensions)
  - Partial: 2 (raylib.h grep — Harness.cpp/ScriptInput.cpp deliberate harness exceptions)
  - N/A: 2 (Task 0 procedural; Task 6 Step 7 workflow)
  - No: 0
- Logic conflicts: **0 real**, **4 stale-doc-only** (Task 5 main.cpp draft superseded by View ownership; worldmap.png → worldmap_base.png filename evolution; deferred-sweep moved to GameController; raylib-self-containment exception list pre-dates the harness layer).
- Net verdict: Plan FULLY DELIVERED, with two superior refinements (ClampToWorld bonus, camera owned by View not main.cpp). No corrective action required.
