# tests/dialog — dialogue parser, layout, state, view

## Files

- `test_dialog_loader.cpp` — markdown parser
- `test_dialog_content_dir.cpp` — `docs/content/` discovery
- `test_dialog_source.cpp` — abstract source impls
- `test_dialog_state.cpp` — branch + cursor
- `test_dialog_layout.cpp` — 28-cell wrapping
- `test_dialog_box_render.cpp` — box render
- `test_dialog_opener.cpp` — interact-to-open
- `test_dialog_skip.cpp` — input skip behaviour

## Dependency direction

- Depends on: controller, entities, gfx, state, ui, world
- Used by: nothing (test leaf)
