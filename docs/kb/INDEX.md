# KB INDEX — 2D RPG / Raylib knowledge base

Local, gitignored. Loaded into team memory via `CLAUDE.md`'s
`@.claude/kb/INDEX.md`. Specialist agents fill the topic files on first
use, following the construction protocol below. Until a file is filled it
carries a `STATUS: STUB` line; an agent that needs it fills it then.

## How each KB file is built (protocol — not optional)

1. **Sources are pinned first** — use only the registry below (verified
   to exist at planning time) plus current-year `WebSearch` for the two
   late-bound topics. No other sources without adding them here.
2. **Fetch, don't recall** — `WebFetch` each source; never write from
   memory. On conflict/unreachable: re-fetch or mark
   `TODO(source-needed)`. Never fabricate.
3. **Distil + ground** — every claim is written as
   `point → where it bites in THIS repo (file:line / class)`. The KB
   points at 《尋傘記》 code, not generic theory.
4. **Cite** — each file ends with `## Sources` listing
   `[title](url) — fetched <date>`. Every non-obvious claim traces to a
   source. Zero uncited assertions.
5. **Verify** — `cpp-architecture-reviewer` spot-checks grounding
   (file:line really matches) and that URLs resolve. Fails → re-fetch.

## Source registry (WebSearch-verified to exist, 2026-05)

- raylib cheatsheet — https://www.raylib.com/cheatsheet/cheatsheet.html
  (`TakeScreenshot`, `Camera2D`, `DrawTexturePro`, input)
- raylib examples — https://www.raylib.com/examples.html
  (`core_2d_camera*`, textures, text)
- raylib wiki — https://github.com/raysan5/raylib/wiki
- raylib headless — raysan5/raylib issue #1376, v6.0 discussion #5783,
  `rlsw` software renderer (HN id=45661638) — for raylib-core headless §
- Kea Sigma Delta "RayLib 2D Challenge" (parts 5/6/8 verified) —
  https://keasigmadelta.com/blog/raylib-2d-challenge-part-6-displaying-tile-map-worlds/
- Game Programming Patterns (R. Nystrom, full text) —
  https://gameprogrammingpatterns.com (Game Loop / State / Event Queue /
  Component / Update Method / Dirty Flag)
- Late-bound via current-year WebSearch: MDN "Square tilemaps"; Unicode
  East Asian Width (CJK full-width, ties to the 28-cell rule).

## Files & when to read

| File | Read when | STATUS |
|---|---|---|
| `raylib-core.md` | touching loop / Camera2D / textures / GL lifetime / screenshots / headless | STUB |
| `topdown-rpg-patterns.md` | tilemap, AABB, `CollisionMask.h` layering, camera deadzone, NPC interaction, quest/flag | STUB |
| `dialogue-systems.md` | editing `DialogLoader`/`docs/content`, branching, CJK wrap, 28-cell box, typewriter | STUB |
| `state-machine-narrative.md` | `SemesterStateMachine`, ripple/latent vars, ending gates | STUB |
| `uiux-rpg.md` | reading harness shots; HUD/dialog-box/sprite/camera/pixel-scale/CJK font critique | STUB |
| `cpp-game-architecture.md` | any structural change — distils STRICT_REVIEW + `awsome_cpp.md`: SOLID, EventBus (see BUGLEDGER H1), ownership, RAII vs GL order | STUB |

Grounding anchors already known (use these when filling):
`main.cpp` loop (53→~60 lines), `include/EventBus.h` (Event Queue),
`SemesterStateMachine` (State), `include/CollisionMask.h` (collision
layers), `gfx/Font.h` (CJK atlas), `View` (Camera2D), `DialogLoader.cpp`
grammar (CLAUDE.md §6).
