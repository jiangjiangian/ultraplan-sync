# src/dialog — dialogue parser, layout, state, and view

Implements the runtime dialogue pipeline declared in `include/dialog/`.
Grammar lives in CLAUDE.md §6 and is mirrored by `DialogLoader.cpp`.

## Files

- `DialogLoader.cpp` — parse `docs/content/*.md`; honour
  `// karma ±N` and `Flag_*` annotations from the SCRIPT_HANDOFF whitelist
- `DialogSource.cpp` — abstract source impls
- `DialogState.cpp` — active branch + cursor mutations
- `DialogLayout.cpp` — 28-full-width-cell wrapper
- `DialogView.cpp` — render box + typewriter
- `DialogOpener.cpp` — interact-to-open entry point

## Dependency direction

- Depends on: entities, gfx, quest
- Used by: controller, entities, harness, quest, state, ui, world
