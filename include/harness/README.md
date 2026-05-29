# harness — autoplay perception + actuation (off in normal play)

Headless harness for the agent team (CLAUDE.md §4). Only active when
`UMBRELLA_SCRIPT` is set; otherwise normal play is bit-for-bit
unchanged. Installs deterministic scripted input + fixed 60 fps step,
screenshots, and a `state.jsonl` per frame.

## Files

- `Harness.h` — BeginFrame/EndFrame/MaybeAttach lifecycle
- `ScriptInput.h` — deterministic `gfx/Input` driver reading a script

## Dependency direction

- Depends on: controller, dialog, entities, gfx, state, world (mostly
  in src/harness/ for jsonl serialisation)
- Used by: nothing — harness sits at the top of the dependency chain
  (only `main.cpp` references it)
