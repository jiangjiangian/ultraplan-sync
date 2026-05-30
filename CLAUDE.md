# CLAUDE.md

Slim project context for《尋傘記：政大山下篇》. Topic detail lives in
path-scoped rules and a dynamic progress file (see "Context layout" below) —
this file holds only what every session needs.

## Project Identity

**《尋傘記：政大山下篇》** (The Lost Umbrella: NCCU Downhill Campus) — a 2D
top-down JRPG in **C++17 + Raylib 5.5**, the OOP Final Project, an extension
of `Assignment #5`, intentionally targeting four GoF design patterns.

The two Chinese design docs in repo root are the **single source of truth**:
- `系統架構與UML分析：尋傘記.md` — UML, state machine, sequence diagram
- `遊戲企劃與敘事架構.md` — chapters, karma/rain, ending matrix, ripples

Read both before adding classes or chapters. Do not restate them here. A
PreToolUse hook blocks edits to these two files — change design intent in
chat first.

## Build & Run

```bash
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5   # configure (FetchContent pulls raylib 5.5)
cmake --build build                                  # compile
./build/OOP_Raylib_Lab                               # run
ctest --test-dir build --output-on-failure           # unit tests (doctest)
```

⚠️ Executable is `OOP_Raylib_Lab` (from `project()` in `CMakeLists.txt:2`),
**not** `OOPFinal`. Do not rename — the grader follows the CMake.

Non-obvious CMake: `GLOB_RECURSE` auto-picks new `.cpp`/`.h` (re-run
`cmake -B build` if the cache is stale); `main.cpp` is excluded from the lib
and linked only into the exe; `resources/` is copied next to the exe so
runtime paths are **relative** (`LoadTexture("resources/assets/...")`);
macOS `-framework Cocoa` is wired; `include/` headers need no prefix
(`#include "Player.h"`).

## Dialog pipeline (architectural invariant)

Dialog is **runtime-loaded** from `docs/content/*.md`, NOT codegen. NPC talk
-> `nccu::dialog::Entries(npcId, SemesterState)` -> `LoadChapter()` parser ->
per-state cache (`Reload()` hot-reloads). The old `gen_dialog.py` /
`DialogData.h` / `gen_dialog` CMake target are **retired — do not
reintroduce**. Deployment implication: `docs/content/` must ship next to the
binary (it is read at runtime, like `resources/`). Editing a chapter `.md`
changes in-game dialog with no rebuild. `docs/content/*.md` is
author-managed — propose content edits in chat (list-diff -> approve ->
apply) before touching them.

## Repository Layout

```
src/            C++ source            include/        C++ headers
tools/          composite_worldmap.py, tiled_to_world.py (collision bake)
resources/design/   reference only — NEVER LoadTexture from here
resources/assets/   game-bound (sprites tiles maps buildings_3d ui audio style)
docs/           ROADMAP.md (Tier-2 SoT) + content/*.md scripts + SCRIPT_HANDOFF.md
系統架構與UML分析：尋傘記.md / 遊戲企劃與敘事架構.md   design SoT
README.md  CMakeLists.txt  .gitignore
```

⚠️ Chinese filenames use a **full-width colon `：`**. Quote in bash:
`"系統架構與UML分析：尋傘記.md"`.

## Hard architectural rules (invariant — always apply)

1. **`Player` must NOT `#include` any concrete umbrella class.** It only
   knows `TransparentUmbrella*`; VTable dispatch does the rest.
2. **UI ↔ data decoupling**: items emit events to an event/UI manager; they
   never call `DrawText` themselves. Rendering goes through `IRenderer`.
3. **Deferred deletion**: mark `isActive = false`, sweep after the loop.
   Never `delete` / erase a GameObject mid-iteration — iterator
   invalidation crashes.

## Reference Documents (read, don't restate)

| Document | Authority over |
|---|---|
| `系統架構與UML分析：尋傘記.md` | class diagram, state machine, sequence, OOD |
| `遊戲企劃與敘事架構.md` | chapters, dialog tone, karma/rain, endings |
| `docs/ROADMAP.md` | Tier-2 build plan (current SoT for what to build next) |
| `docs/SCRIPT_HANDOFF.md` | content↔code contract (karma/money/rain/flags) |
| `README.md` | grading rubric |
| `resources/design/山下校園.jpg` + `GameMap.png` | spatial layout (red line = playable boundary) |

## Known Pitfalls (general)

1. Executable is `OOP_Raylib_Lab`, not `OOPFinal`.
2. Chinese filenames need quoted paths (full-width colon).
3. Parent dir `三下/oop/` is **also a git repo** (unrelated weekly hw) —
   always run `git` from inside `assignment-5-jiangjiangian/`.
4. `resources/design/` is 6+ MB reference — only `resources/assets/` is ever
   opened by raylib.
5. Save-state format undecided — propose simple JSON in `resources/save/`
   (gitignored) when implementing.
6. Never `rm -rf build` — trust incremental `cmake --build build`.
7. `CLAUDE.md` and everything under `.claude/` are gitignored (local-only,
   never pushed). Pushed files must not name external tools/services — scan
   before any push.

(Asset / image-gen MCP pitfalls live in `.claude/rules/asset-generation.md`,
loaded only when touching `resources/assets/`.)

## Context layout (how this project's memory is organized)

Per Claude Code's memory model, detail is split so each session loads only
what it needs:

- `.claude/rules/architecture.md` — inheritance tree, GoF→files, state
  machine. Path-scoped: loads when editing `include/` `src/` `tests/`.
- `.claude/rules/asset-generation.md` — image-gen MCP workflow, style
  anchors, prompt rules, asset pitfalls. Path-scoped: loads when touching
  `resources/assets/` or `tools/composite_worldmap.py`.
- `.claude/rules/workflow.md` — per-task workflow, verification gate, commit
  conventions, tooling. Always loaded.
- `.claude/progress.md` — **dynamic project state. Not auto-loaded.**
- Auto memory (`~/.claude/projects/.../memory/`) — cross-session learnings
  Claude writes itself. CLAUDE.md = stable instructions; progress.md =
  changing state; memory = learnings. Don't mix the three.

**Session protocol:** at the start of a work session, read
`.claude/progress.md`. When you finish meaningful work, update it
(last-updated date, current phase, what changed, next action; keep < 60
lines). This replaces the old stale "Current Progress" snapshot.
