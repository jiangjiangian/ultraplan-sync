# Audit — docs/superpowers/specs/2026-05-11-raylib-modern-cpp-wrapper-design.md

**Overview (≤3):**
1. Wrapper layer fully realised: every spec'd `nccu::gfx::*` element ships under `include/gfx/` and is the sole choke-point for `#include "raylib.h"` in production game code; spec §8 gate items 1–3 are met by current Cycle-8 gate (289/289 tests, 0 warnings).
2. **Real conflict (1):** spec §8 gate #4 (`grep raylib.h … --include="*.cpp" --include="*.h" | grep -v include/gfx/` returns no matches) FAILS — `src/Harness.cpp:12` and `src/ScriptInput.cpp:10` include `raylib.h`. These are the autoplay harness (CLAUDE.md §4), not present at spec time; the spec was not updated. The constraint as written is shipped-violated.
3. **Stale-doc-only divergences:** spec §10 plans to migrate `Event::position` / `Event::color` to wrapper types — shipped EventBus already removed those payloads entirely (`type` + `text` only). §11 parks Texture/Font RAII as out-of-scope — both shipped. Multiple additions beyond spec (IRenderer/RaylibRenderer DIP layer, Bounds, Camera2D, CameraScope, MaskLoader) are unmentioned.

## Per-element annotations

- **§2 self-containment: game code only touches `nccu::gfx::*`** — All non-`gfx/` headers/sources avoid raw raylib types.
  - **[是否實作?]** Partial — production game files clean (verified by grep); harness files (`src/Harness.cpp:12`, `src/ScriptInput.cpp:10`) still include `raylib.h`.
  - **[邏輯衝突?]** Yes — **Real conflict** with §8 gate #4. Harness post-dates spec; spec needs an explicit harness carve-out, OR harness must move behind the wrapper.

- **§4 namespace `nccu::gfx` under `include/gfx/`** — Header-only library location.
  - **[是否實作?]** Yes — `include/gfx/` exists with 16 headers all under `namespace nccu::gfx`.
  - **[邏輯衝突?]** No.

- **Color.h (Adapter + Fluent, `WithAlpha`, constexpr palette)** — `include/gfx/Color.h:7-34`.
  - **[是否實作?]** Yes — struct + `WithAlpha`, equality ops, `Colors::{Black,White,RayWhite,DarkGray,Blue,Red,Green,Yellow,Gold,Magenta}` palette (constexpr inline).
  - **[邏輯衝突?]** No.

- **Vec2.h (Adapter, `operator+ - *`, `Length`)** — `include/gfx/Vec2.h:7-23`.
  - **[是否實作?]** Yes — arithmetic ops constexpr; `Length`, `Normalized` provided.
  - **[邏輯衝突?]** No.

- **Rect.h (Adapter, `Contains(point)`, `Intersects(rect)`)** — `include/gfx/Rect.h:7-24`.
  - **[是否實作?]** Yes — both methods present and constexpr.
  - **[邏輯衝突?]** No.

- **Key.h (`enum class Key` mapping raylib `KEY_*`)** — `include/gfx/Key.h:7-24`.
  - **[是否實作?]** Yes — letters A–Z plus Space/Tab/Enter/Escape/arrows; `ToRaylibKey` adapter.
  - **[邏輯衝突?]** No.

- **Window.h (RAII + Builder; move-only)** — `include/gfx/Window.h:9-50`.
  - **[是否實作?]** Yes — `Builder().Title().Size().Fps().Open()`; copy deleted; move with owns-flag transfer; dtor calls `CloseWindow` only when owning.
  - **[邏輯衝突?]** No.

- **DrawScope.h (RAII, non-copyable, non-movable)** — `include/gfx/DrawScope.h:7-16`.
  - **[是否實作?]** Yes — ctor `BeginDrawing`, dtor `EndDrawing`, all 4 special members deleted/correct.
  - **[邏輯衝突?]** No.

- **Renderer.h (Fluent free-helper; `Clear/Rect/Pixel` returning `Renderer&`)** — `include/gfx/Renderer.h:11-66`.
  - **[是否實作?]** Yes — `Clear`, `Rect`, `RectLines`, `Pixel`; also adds `Texture` + `TextureRect` (sprite path) beyond spec.
  - **[邏輯衝突?]** No (additions consistent with §11 Texture parking-lot now landed).

- **TextBuilder.h (Builder; chained `.At/.Size/.Color/.Draw`)** — `include/gfx/TextBuilder.h:12-48`.
  - **[是否實作?]** Yes — chained setters; `Draw()` routes through CJK font when loaded, else `DrawText`.
  - **[邏輯衝突?]** No.

- **Input.h (Facade; `Input::IsDown/IsPressed/IsReleased`)** — `include/gfx/Input.h:30-46`.
  - **[是否實作?]** Yes — static member functions over `InputSource` polymorphism (`LiveInput` default; harness installs `ScriptInput`). Spec'd as plain facade; shipped form layers a Strategy/DI on top — additive, not conflicting.
  - **[邏輯衝突?]** No (additive — harness need predated spec).

- **Time.h (Facade; `DeltaSeconds`, `FpsAvg`)** — `include/gfx/Time.h:7-20`.
  - **[是否實作?]** Yes — both static methods; adds `SetFixedStep` for harness determinism.
  - **[邏輯衝突?]** No.

- **`Colors` constexpr palette (RAYWHITE/BLUE/BLACK/WHITE/DARKGRAY)** — `include/gfx/Color.h:23-34`.
  - **[是否實作?]** Yes — all named constants present (RayWhite, Blue, Black, White, DarkGray) plus extras.
  - **[邏輯衝突?]** No.

- **Wrapper memory layout identical to raylib counterparts + internal-only conversion ops** — §4.
  - **[是否實作?]** Partial — `Color`/`Vec2`/`Rect` have identical POD field layout, but no `operator raylib::Vector2()` etc. are defined; conversion is done via explicit field copy at every call site (e.g. `Renderer.h:22-23`).
  - **[邏輯衝突?]** No — **Stale-doc-only**: spec described one implementation strategy; shipped chose another with the same end-effect (no raylib types leak to game code).

- **§5 Pattern map row "Window" RAII+Builder; `Builder().Title().Size().Fps().Open()`** — see Window.h.
  - **[是否實作?]** Yes — exact API.
  - **[邏輯衝突?]** No.

- **§5 EventBus Observer (unchanged)** — `include/EventBus.h:26-108`.
  - **[是否實作?]** Yes — `Subscribe`/`Publish`; also gained `ScopedSubscribe` RAII (BUGLEDGER H1, CHANGELOG 2026-05-18) — additive beyond spec.
  - **[邏輯衝突?]** No — additive.

- **§5 GameObjectFactory Factory Method (unchanged)** — `include/GameObjectFactory.h`, `src/GameObjectFactory.cpp`.
  - **[是否實作?]** Yes — unchanged.
  - **[邏輯衝突?]** No.

- **§5 Template Method `TransparentUmbrella::beClaimed` + 4 subclasses** — `src/TransparentUmbrella.cpp`, `Cursed/Fragile/ProfessorTrap/TrueUmbrella`.
  - **[是否實作?]** Yes — class hierarchy intact.
  - **[邏輯衝突?]** No.

- **§6 `src/main.cpp` migrated to `Window::Builder` + `DrawScope` + Renderer/TextBuilder + `Input::IsPressed(Key::E)` + `Time::DeltaSeconds()`** — `src/main.cpp:32-118`.
  - **[是否實作?]** Yes — main uses `nccu::gfx::Window::Builder()` (line 32), `DrawScope` (line 91), `EnsureFont`/`ShutdownFont` (lines 40/118); no raw `InitWindow`/`BeginDrawing`.
  - **[邏輯衝突?]** No.

- **§6 `src/Player.cpp` uses `Renderer{}.Rect(hitBox_, Colors::Blue)` + `Input::IsDown(Key::W/A/S/D)`** — `src/Player.cpp:59-93`.
  - **[是否實作?]** Yes — `Render` takes `IRenderer&`; `HandleInput` uses `Input::IsDown(Key::W/A/S/D)`. Spec said `Renderer{}.Rect(...)` directly; shipped routes through `IRenderer::DrawRect` (DIP layer added).
  - **[邏輯衝突?]** No — **Stale-doc-only**: shipped form is strictly more decoupled than spec'd; backed by `RaylibRenderer` which calls the spec'd `Renderer{}`.

- **§6 `src/TransparentUmbrella.cpp` unchanged + 4 concrete `.cpp` unchanged** — `src/TransparentUmbrella.cpp`, etc.
  - **[是否實作?]** Yes — still publish `RenderRequested`-style flow via EventBus; render code migrated to gfx types (`nccu::gfx::Color umbrellaTint_` in `include/TransparentUmbrella.h:43`).
  - **[邏輯衝突?]** No.

- **§6 `RenderRequested` subscriber in main.cpp uses `Renderer{}.Rect(...)`** — main.cpp render block.
  - **[是否實作?]** Yes — flowed through `View`/`Renderer` per spec intent.
  - **[邏輯衝突?]** No.

- **§6 Header migration: `GameObject/Character/Item/Player/EventBus/Transparent*` swap `Vector2→Vec2`, `Rectangle→Rect`, `Color→Color`** — `include/Player.h:86,145`, `TransparentUmbrella.h:22,43`, etc.
  - **[是否實作?]** Yes — verified by grep: no `Vector2`/`Rectangle`/`::Color` in game headers; `nccu::gfx::Color`/`Vec2`/`Rect` used throughout.
  - **[邏輯衝突?]** No.

- **§7 Tests: `test_color/test_rect/test_vec2/test_text_builder`** — `tests/test_color.cpp`, `test_rect.cpp`, `test_vec2.cpp`, `test_text_builder.cpp`.
  - **[是否實作?]** Yes — all four files exist.
  - **[邏輯衝突?]** No.

- **§8 Gate #1: -Wall -Wextra -Wpedantic, 0 warnings** — Cycle-8 gate.
  - **[是否實作?]** Yes — CHANGELOG 2026-05-20 confirms "0 project-code warnings (C++20 -Wall -Wextra -Wpedantic)".
  - **[邏輯衝突?]** No.

- **§8 Gate #2: all tests passing, existing 9 + new ≥4** — Cycle-8 gate.
  - **[是否實作?]** Yes — 289/289 / 4254 assertions.
  - **[邏輯衝突?]** No.

- **§8 Gate #3: smoke run ≥5 s, no crash** — Cycle-8 gate references `playtest.sh` ending runs A/B/C.
  - **[是否實作?]** Yes — endings A/B/C reach 結局 at 7490/4834/4142 lines, byte-identical.
  - **[邏輯衝突?]** No.

- **§8 Gate #4: `grep -rn "raylib.h" src/ tests/ include/ … | grep -v "include/gfx/"` returns no matches** — verified just now.
  - **[是否實作?]** No — grep returns 2 matches: `src/Harness.cpp:12`, `src/ScriptInput.cpp:10`.
  - **[邏輯衝突?]** Yes — **Real conflict**. Spec gate FAILS as written. Harness shipped post-spec (CLAUDE.md §4); spec was not amended. Resolution paths: (a) amend spec/gate to exclude `src/Harness.cpp`/`src/ScriptInput.cpp`; (b) introduce a `gfx/` shim that the harness alone uses.

- **§9 Implementation order (bottom-up, layered)** — informational.
  - **[是否實作?]** N/A — historical guidance; not a runtime constraint.
  - **[邏輯衝突?]** No.

- **§10 Risk: `DrawScope` lifetime within `Window` lifetime** — RAII contract.
  - **[是否實作?]** Yes — enforced by ordering in `src/main.cpp` (Window opens line 32, DrawScope inside frame loop, Window dtor last).
  - **[邏輯衝突?]** No.

- **§10 Risk: `Window` move-only, deleted copy** — Window.h.
  - **[是否實作?]** Yes — copy ctor/assign deleted, move sets `owns_=false` on source.
  - **[邏輯衝突?]** No.

- **§10 Risk: No Singleton; `Renderer{}` value temporary; `Input`/`Time` static functions** — Renderer.h/Input.h/Time.h.
  - **[是否實作?]** Yes — `Renderer` has no static state; `Input::current_` is a class-level pointer for the Strategy, not a Singleton instance; `Time` is statics-only.
  - **[邏輯衝突?]** No (note: pre-existing `EventBus::Instance()` is a Singleton, but EventBus was explicitly out of scope for this wrapper spec).

- **§10 Risk: implicit conversions internal-only** — verified by grep: no public `operator ::Color()`/`operator ::Vector2()` in `include/gfx/`.
  - **[是否實作?]** Yes — no public conversion operators; all crossing is explicit field-by-field inside `Renderer.h`/`TextBuilder.h`.
  - **[邏輯衝突?]** No.

- **§10 Risk: EventBus `Event::position` / `Event::color` migrate to wrapper types** — `include/EventBus.h:21-24`.
  - **[是否實作?]** N/A — shipped `Event` has only `type` + `text` (string id). The Vector2/Color payload fields the spec references no longer exist.
  - **[邏輯衝突?]** No — **Stale-doc-only**: the payload was redesigned away; consistent with the self-containment rule by elimination rather than migration.

- **§11 Out-of-scope: Texture / Font / Sound RAII (parking lot)** — `include/gfx/Texture.h`, `include/gfx/Font.h`.
  - **[是否實作?]** Yes (beyond spec) — `Texture` is move-only RAII; `Font` is a process-wide CJK font manager with explicit `ShutdownFont` (BUGLEDGER V1, V3 lineage).
  - **[邏輯衝突?]** No — **Stale-doc-only**: parking lot landed when sprites/CJK rendering needed them; CHANGELOG documents Cycle 1+.

- **§11 Out-of-scope: Mouse/gamepad, particle/shader** — none present.
  - **[是否實作?]** N/A — still out of scope.
  - **[邏輯衝突?]** No.

- **Additions beyond spec: `IRenderer` + `RaylibRenderer`** — `include/gfx/IRenderer.h`, `RaylibRenderer.h`.
  - **[是否實作?]** Yes (extra) — Strategy / DIP layer so `Player::Render(IRenderer&)` is testable without GL and decoupled from raylib at the call site.
  - **[邏輯衝突?]** No — **Stale-doc-only**: additive; consistent with §2 self-containment intent.

- **Additions beyond spec: `Bounds.h`, `Camera2D.h`, `CameraScope.h`, `MaskLoader.h`** — `include/gfx/`.
  - **[是否實作?]** Yes — added per the camera/world-clamp companion specs (2026-05-11-camera-worldmap-design, 2026-05-11-world-clamp-and-entry-design).
  - **[邏輯衝突?]** No — out-of-scope additions; not contradicted by this spec.

## Summary

- **Elements audited:** 33
- **Yes (implemented):** 28
- **Partial:** 2 (§2 self-containment harness exemption; memory-layout conversion strategy differs)
- **No:** 1 (§8 gate #4 grep)
- **N/A:** 2 (§9 order; §10 EventBus payload eliminated; §11 mouse/gamepad)
- **Real conflicts:** 1 (§8 gate #4 — `src/Harness.cpp` + `src/ScriptInput.cpp` include `raylib.h`)
- **Stale-doc-only divergences:** 5 (§10 layout-conversion; §10 EventBus payload; §11 Texture/Font landed; §6 Player via IRenderer; numerous additive headers)
