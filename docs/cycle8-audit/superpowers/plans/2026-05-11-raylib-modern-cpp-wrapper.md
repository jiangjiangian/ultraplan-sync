# Audit — docs/superpowers/plans/2026-05-11-raylib-modern-cpp-wrapper.md

**Overview (≤3):**
1. The wrapper layer **shipped and is in use everywhere** — `include/gfx/` exists with Color/Vec2/Rect/Key/Time/Input/Window/DrawScope/Renderer/TextBuilder, plus extras the plan never mentioned (Texture, Font, Camera2D, CameraScope, IRenderer/RaylibRenderer, MaskLoader, Bounds). Tests `test_color/vec2/rect/text_builder.cpp` all exist.
2. The **rendering contract diverged**: plan called for `GameObject::Draw()` over a global `Renderer{}` instance using `EventType::RenderRequested` plumbing; shipped code uses `GameObject::Render(nccu::gfx::IRenderer&)` (an Adapter behind an interface) and **deleted** RenderRequested entirely. The shipped EventBus payload is a bare `{type, text}` — `Event::position` and `Event::color` from the plan are absent.
3. Implicit `operator ::Color` / `::Vector2` / `::Rectangle` conversions that the plan baked into Color/Vec2/Rect are **not present** in shipped headers; Renderer.h instead constructs raylib structs by hand at call sites. Stale-doc-only — the wrapper still works, just less elegantly than the plan drew.

## Per-element annotations

- **`nccu::gfx::Color` struct + `WithAlpha` + `Colors::` palette** — RGBA bundle with named constants.
  - [是否實作?] Yes — `include/gfx/Color.h:1-38` (adds `Colors::Gold` over the plan's set).
  - [邏輯衝突?] Yes (Stale-doc-only) — no implicit `operator ::Color` conversion (`Color.h:13-21`); Renderer constructs `::Color{c.r,c.g,c.b,c.a}` explicitly at every call (`Renderer.h:14,23,30`). Behaviour identical.

- **`tests/test_color.cpp`** — 5 cases covering default/aggregate/WithAlpha/palette/equality.
  - [是否實作?] Yes — `tests/test_color.cpp` (43 lines).
  - [邏輯衝突?] No.

- **`nccu::gfx::Vec2` + arithmetic + `Length` + `Normalized`** — float xy bundle.
  - [是否實作?] Yes — `include/gfx/Vec2.h:1-27`.
  - [邏輯衝突?] Yes (Stale-doc-only) — no `operator ::Vector2()`; Renderer wraps explicitly.

- **`tests/test_vec2.cpp`** — 5 cases (default/aggregate/arith/Length/Normalized).
  - [是否實作?] Yes — `tests/test_vec2.cpp` (44 lines).
  - [邏輯衝突?] No.

- **`nccu::gfx::Rect` + `Contains` + `Intersects`** — float xywh.
  - [是否實作?] Yes — `include/gfx/Rect.h:1-28`.
  - [邏輯衝突?] Yes (Stale-doc-only) — no `operator ::Rectangle()`.

- **`tests/test_rect.cpp`** — 6 cases.
  - [是否實作?] Yes — `tests/test_rect.cpp` (43 lines).
  - [邏輯衝突?] No.

- **`nccu::gfx::Key` enum + `ToRaylibKey`** — type-safe key map.
  - [是否實作?] Yes — `include/gfx/Key.h:1-28`. Plan-extra `Tab` added (`Key.h:15`).
  - [邏輯衝突?] No.

- **`nccu::gfx::Time::DeltaSeconds`/`FpsAvg`** — frame-time facade.
  - [是否實作?] Yes — `include/gfx/Time.h:7-20`.
  - [邏輯衝突?] Yes (Stale-doc-only) — extra `SetFixedStep()` static for deterministic harness replay (`Time.h:12,19`); behaviour-additive only.

- **`nccu::gfx::Input::IsDown/IsPressed/IsReleased`** — keyboard facade.
  - [是否實作?] Yes — `include/gfx/Input.h:30-46`.
  - [邏輯衝突?] Yes (Stale-doc-only) — gained a polymorphic `InputSource` + `SetSource()` (autoplay harness, CLAUDE.md §4). Game code path unchanged.

- **`nccu::gfx::Window` RAII + `Builder`** — owns init/close.
  - [是否實作?] Yes — `include/gfx/Window.h:1-54`. Byte-equivalent to plan.
  - [邏輯衝突?] No.

- **`nccu::gfx::DrawScope` RAII** — BeginDrawing/EndDrawing.
  - [是否實作?] Yes — `include/gfx/DrawScope.h:7-16`.
  - [邏輯衝突?] No.

- **`nccu::gfx::Renderer` fluent (Clear/Rect/RectLines/Pixel)** — primitive draw helper.
  - [是否實作?] Yes — `include/gfx/Renderer.h:11-66`.
  - [邏輯衝突?] Yes (Stale-doc-only) — also gained `Texture` + `TextureRect` overloads (sprite-sheet support; not in plan but needed for shipped sprites).

- **`nccu::gfx::TextBuilder` Builder** — `At/Size/Color/Draw`.
  - [是否實作?] Yes — `include/gfx/TextBuilder.h:12-48`.
  - [邏輯衝突?] Yes (Stale-doc-only) — `Draw()` now routes through `CJKFont()` when loaded (`TextBuilder.h:25-30`), falls back to `::DrawText`. Strict superset.

- **`tests/test_text_builder.cpp`** — 3 fluent-state cases.
  - [是否實作?] Yes — `tests/test_text_builder.cpp` (27 lines).
  - [邏輯衝突?] No.

- **Task 11: migrate `EventBus.h` `Event::position` / `Event::color` to `Vec2`/`Color`** — wrapper types in event payload.
  - [是否實作?] No — `include/EventBus.h:21-24` declares only `{type, text}`; the gfx fields were not added.
  - [邏輯衝突?] Yes (Real conflict) — plan-vs-shipped: shipped path drops position/color from the payload entirely. Combined with the loss of `EventType::RenderRequested` (see next), the planned "publish a render request, main subscribes and Renderer::Rect-draws it" pipeline does not exist.

- **`EventType::RenderRequested` + main.cpp render-subscription** — planned indirection.
  - [是否實作?] No — `EventBus.h:10-19` enumerates `UmbrellaClaimed/KarmaChanged/ShowMessage/EnteredBuilding/PickupAcquired`. main.cpp has no `EventType::RenderRequested` subscriber.
  - [邏輯衝突?] Yes (Real conflict) — replaced by direct polymorphic `GameObject::Render(IRenderer&)` (`GameObject.h:20`, `TransparentUmbrella.cpp:31`). Same end-state visually, different architecture.

- **Game-header migration to `Vec2`/`Rect`/`Color` + drop `raylib.h`** — purge raylib from game headers.
  - [是否實作?] Yes — `GameObject.h:14,23,29,60-61`, `Character.h:9-26,34`, `Item.h:8`, `Player.h:13,86-87,144-146`, `TransparentUmbrella.h:21-43`, `TrueUmbrella.h:9-11`, `ProfessorTrapUmbrella.h:9-10`, `GameObjectFactory.h:29`. `grep "raylib.h" include/ | grep -v gfx/` returns empty.
  - [邏輯衝突?] No.

- **`GameObject::Draw() const = 0`** — virtual draw method.
  - [是否實作?] No — shipped is `virtual void Render(nccu::gfx::IRenderer&) const = 0` (`GameObject.h:20`).
  - [邏輯衝突?] Yes (Real conflict) — different signature/name; Adapter pattern via injected `IRenderer` not a global Renderer{} instance.

- **`Interact(Player&)` non-null contract per CLAUDE.md §5** — pointer-vs-reference debate.
  - [是否實作?] No — shipped `Interact(Player* initiator)` (`GameObject.h:21`, `Player.h:17`, `TransparentUmbrella.h:37`).
  - [邏輯衝突?] Yes (Real conflict, but pre-existing) — plan calls for `Player*` matching code; CLAUDE.md §5 calls for `Player&`. Discrepancy is with CLAUDE.md, not with this plan.

- **`src/Player.cpp` Draw via `nccu::gfx::Renderer{}.Rect(...)`** — Renderer call site.
  - [是否實作?] Partial — uses `renderer.DrawRect(hitBox_, Colors::Blue)` via injected `IRenderer&` (`Player.cpp:59-62`), not a freshly constructed `Renderer{}`. HandleInput uses `gfx::Input`/`Key` as planned (`Player.cpp:87-89`).
  - [邏輯衝突?] Yes (Stale-doc-only) — different Renderer plumbing, same behaviour.

- **`src/TrueUmbrella.cpp` publish with `Vec2`/`Color` payload** — plan-style Event construction.
  - [是否實作?] No — `TrueUmbrella.cpp:17` publishes `Event{EventType::UmbrellaClaimed, "TrueUmbrella"}` (text-only).
  - [邏輯衝突?] Yes (Real conflict, follows from EventBus divergence). Same for Fragile/ProfessorTrap/Cursed.

- **`src/main.cpp` Window/DrawScope/Renderer/TextBuilder/Input/Time wiring** — composition root.
  - [是否實作?] Partial — uses `Window::Builder` (`main.cpp:32-36`), `DrawScope` (`main.cpp:91`), `EnsureFont`/`ShutdownFont` (extras not in plan). Game-loop body delegates to `World/View/GameController` MVC (per CLAUDE.md §5), so `Renderer/TextBuilder/Input/Time/HUD` calls in plan's main.cpp body do not appear here — they moved into `View.cpp`/`GameController.cpp`.
  - [邏輯衝突?] Yes (Stale-doc-only) — MVC refactor (Cycles 1–7, CHANGELOG `b26aa9a`) absorbed plan's main-loop snippet; same visible behaviour.

- **Task 14: `EventBus.cpp` raylib-free** — verification step.
  - [是否實作?] Yes — `grep "raylib\|Vector2\|Rectangle" src/EventBus.cpp` returns nothing.
  - [邏輯衝突?] No.

- **Task 15 verification gate (zero warnings · ctest green · raylib.h grep clean · forbidden-token grep clean)** — final acceptance.
  - [是否實作?] Partial — `grep "raylib.h" src/ include/ tests/ | grep -v "include/gfx/"` returns ONLY 3 src files (`Harness.cpp:12`, `ScriptInput.cpp:10`, `MessageView.cpp:34`-comment) that legitimately need raylib for input keymaps and CJK pen-advance; tests/ and include/ are clean. CHANGELOG records the gate as green throughout Cycles 1–8.
  - [邏輯衝突?] Yes (Stale-doc-only) — plan's "no game-side raylib.h" is met for headers but two legacy `.cpp` (Harness, ScriptInput) still include it.

- **File-structure block: 10 wrapper headers + 4 tests** — directory layout.
  - [是否實作?] Yes (superset) — `ls include/gfx/` shows 19 headers; only the 10 planned files + Font/Texture/Camera2D/CameraScope/IRenderer/RaylibRenderer/MaskLoader/Bounds extras.
  - [邏輯衝突?] No.

- **"`CMakeLists.txt` — no change needed (GLOB picks up `include/gfx/*.h`)"** — CMake assumption.
  - [是否實作?] Yes — incremental build confirmed by CHANGELOG `6bf5a5e` policy shim; no manual CMake list edits required.
  - [邏輯衝突?] No.

- **Squash-before-merge guidance (Task 15 Step 7)** — git workflow tip.
  - [是否實作?] N/A — observation only; granular commits were kept (CHANGELOG history visible).
  - [邏輯衝突?] No.

## Summary

- Elements audited: **27**
- Yes: 14 · Partial: 4 · No: 6 · N/A: 1
- Conflicts: **15** (Stale-doc-only: 10 · Real conflict: 5)
- Real conflicts cluster around the abandoned `Event::position`/`color`/`RenderRequested` indirection, replaced by the `IRenderer`-Adapter rendering path (cleaner architecture; ships in production). All other divergences are stale-doc cosmetics (implicit conversions absorbed into explicit Renderer call sites; MVC absorbed main.cpp body).
