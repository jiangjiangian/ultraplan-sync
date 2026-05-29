# 《尋傘記》 Coding & Naming Conventions

The single source of truth for how this codebase is written, referenced by
every refactor batch. Extends `awsome_cpp.md` (C++ practice) and `CLAUDE.md`
§5–§6 (red lines, dialogue grammar). When this file and habit disagree, **this
file wins** — fix the code.

> Refactor discipline: a rename/move must change *structure, not behavior*.
> Every batch is gated on the byte-identical `state.jsonl` oracle
> (`/tmp/refactor_base`, sha `3ea809bb…`) + 561 tests + 0 warnings +
> `dialog_lint` 0. A divergence on a structural step is a bug — back out.

---

## 1. Files & directories
- One primary type per file; filename = the type, **`PascalCase.{h,cpp}`**
  (`DialogLoader.h`/`.cpp`). `main.cpp` is the sole lowercase file.
- Tests: `tests/<domain>/test_<unit>.cpp` (doctest).
- Layout is **by-domain**, `include/` + `src/` mirrored, one-way layering:
  `engine/ ← game/{world,narrative,entities,systems,quests} ← ui/ ← app/`.
  (See the refactor plan for the full tree.)

## 2. Namespaces
- Flat **`nccu`** root; keep the existing **`nccu::gfx`**. Do NOT add
  per-domain namespaces — the *folders* carry domain separation; extra
  namespaces only add `using`/qualification churn.

## 3. Types, interfaces, enums
- Types: **`PascalCase`** (`GameObject`, `DialogState`).
- Interfaces (pure-virtual / role): **`I`-prefix** (`IRenderer`, `ISystem`,
  `IMortal`, `IUpdatable`).
- Enums: `PascalCase` type **and** `PascalCase` enumerators
  (`enum class HeldUmbrella { None, True, Cursed … }`). Prefer `enum class`.

## 4. Members & functions
- Data members: **trailing underscore** (`position_`, `hp_`, `isActive_`).
- Methods & free functions: **`PascalCase`** (`Update`, `Render`,
  `BuildInventoryRows`). Legacy outlier `beClaimed` is grandfathered **until
  the entities batch**, then renamed → `BeClaimed` (virtual chain renamed
  together).
- Pass non-null model objects **by reference** (`Player&`), nullable by
  pointer (`Player*`). `[[nodiscard]]` on pure queries; `noexcept` where it
  holds; `const`-correct.

## 5. Constants
- Compile-time constants: **`kPascalCase`** (`kMaxHp`,
  `kInventoryRowsPerPage`), `inline constexpr`.

## 6. FLAGS — one registry (the highest-value rule)
- All event flags live in **one** registry header (`game/narrative/Flags.h`).
- Each flag: `inline constexpr const char* kFlag<Name> = "Flag_<Name>";`
  where **the constant suffix EQUALS the string suffix**.
  - ✅ `kFlagBookwormRecovered = "Flag_BookwormRecovered"`
  - ❌ `kFlagBookwormWoken     = "Flag_Bookworm"`  (current drift — fix it)
- **Zero raw `"Flag_X"` string literals** anywhere in code or in `DialogChoice`
  payloads — always reference the `kFlag*` constant.
- The flag *whitelist* in `docs/content` (the SCRIPT_HANDOFF set) and the
  registry must match; `dialog_lint.py` + `tools/text_map.py` are extended to
  enforce this at gate time.

## 7. Events
- One `engine/events/Events.h`: `EventType` enumerators `PascalCase`
  (`UmbrellaClaimed`, `ShowMessage`, `EnteredBuilding`). Publish through the
  `EventBus` singleton; subscribers capture `bus`, never re-`Instance()` inside
  a handler.

## 8. Includes & dependency rules (red lines)
- Include by **domain path**: `#include "game/narrative/DialogLoader.h"`.
- One-way layering only: **`engine/` never includes `game/`**; `game/` never
  includes `ui/` or `app/`; `ui/` reads model headers **read-only** (never
  mutates `World`/`Player`); `app/` composes everything.
- **Model/Items never call raylib draw** (`DrawText`/`DrawTexture`) — route
  rendering through the injected `IRenderer`, and side-effects through the
  `EventBus`.
- The **harness seam** (`engine/platform`) must keep normal play
  bit-identical when `UMBRELLA_SCRIPT` is unset.

## 9. Memory & lifetime
- Own `GameObject`s via `std::unique_ptr`; never `delete` mid-iteration —
  mark `isActive_ = false` and let `World::Sweep()` erase at end-of-frame.
  `objects_.front()` stays the Player. No `dynamic_cast` (use the role
  accessors `As*()`); `static_cast` only behind a documented invariant.

## 10. raylib resource lifecycle (RAII) — codifies the existing discipline
These are not aspirational; they describe how `include/gfx/` already works and
make it the standard for every new raylib-backed resource (incl. audio).
- **R1 — one move-only RAII wrapper per raylib resource type.** Copy deleted,
  move transfers ownership, dtor `Unload`s. Pattern: `gfx::Texture`
  (`include/gfx/Texture.h`). Applies to `Texture2D`, `Font`, and (new)
  `Sound`/`Music`. Prefer a dedicated wrapper over a bare
  `unique_ptr<…, Deleter>` (see R3 for why).
- **R2 — owner vs view.** Exactly one *owner* per GPU/audio id (its dtor
  `Unload`s); any number of cheap non-owning *views* may alias it and never
  `Unload` (no double-free). Pattern: `gfx::Texture` owner/view + the load-once
  `detail::TextureCache`.
- **R3 — explicit teardown, never a static dtor (the load-bearing rule).**
  Process-lifetime caches are function-local statics, but are emptied by an
  explicit `Shutdown*()` called from `main()` *while the window/GL context (and
  audio device) is still alive — before* `~Window`/`CloseWindow`. A
  static/Meyers dtor runs *after* `main()` returns, i.e. after `CloseWindow`
  tore down the GL context, and `Unload*` on a dead context is UB — so a
  `std::unique_ptr<Texture2D, UnloadDeleter>` at static scope is **wrong**
  here. See `gfx::ShutdownTextureCache` / `gfx::ShutdownFont` and the teardown
  order in `main.cpp`; audio adds `gfx::ShutdownAudio()` before `~AudioDevice`.
- **R4 — scope guards for paired Begin/End.** `gfx::DrawScope`
  (`Begin/EndDrawing`), `gfx::CameraScope` (`Begin/EndMode2D`) — RAII so an
  early `return` cannot leave them unbalanced. Add new paired calls the same
  way.
- **R5 — `raylib.h` is confined to `engine/`** (render/platform/input/audio).
  `game/` and `ui/` never include `raylib.h`, never call `Load*`/`Unload*`/
  `Draw*` directly — they route through the wrappers, `IRenderer`, and the
  `EventBus`.
- **R6 — Audio mirrors R1–R3.** `engine/audio/AudioDevice` RAII
  (`InitAudioDevice` ctor / `CloseAudioDevice` dtor); `Sound`/`Music` move-only
  handles + a load-once `AudioManager` cache; `ShutdownAudio()` from `main()`
  before `~AudioDevice`.
- **raylib flags confinement.** raylib config/input enums (`FLAG_*`, `KEY_*`,
  `ConfigFlags`) stay inside `engine/` (Window/Input); never leak into `game/`
  logic — game code uses the typed `InputSource` abstraction. (Game *narrative*
  flags are the separate `kFlag*` registry, §6 — do not conflate the two.)
