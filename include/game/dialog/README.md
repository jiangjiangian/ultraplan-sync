# dialog — runtime dialogue loading, layout, state, and view

Dialogue is loaded from `docs/content/*.md` at runtime (CLAUDE.md §6).
This bucket parses that grammar, manages the active dialog state, and
renders the 28-cell CJK box.

## Files

- `DialogLoader.h` — parses markdown into branches; honours
  `// karma ±N` and `Flag_*` annotations
- `DialogSource.h` — abstract source of dialog lines (NPC, building, ...)
- `DialogState.h` — currently-playing dialog branch + cursor
- `DialogLayout.h` — 28-full-width-cell wrapping
- `DialogView.h` — render box + typewriter
- `DialogOpener.h` — interact-to-open entry point

## Dependency direction

- Depends on: state (headers); plus entities, gfx, quest in src/dialog/
- Used by: controller, entities, harness, quest, state, ui, world
