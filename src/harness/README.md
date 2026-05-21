# src/harness — autoplay harness implementation

Implements the perception + actuation harness (CLAUDE.md §4). Off
unless `UMBRELLA_SCRIPT` is set, in which case it installs a
deterministic scripted input source, a fixed 60 fps step, periodic
screenshots, and per-frame `state.jsonl` lines.

## Files

- `Harness.cpp` — BeginFrame/EndFrame/MaybeAttach lifecycle + jsonl
  serialiser (player, karma, money, rain, flags, dialog, semester,
  objects, events)
- `ScriptInput.cpp` — script-driver impl of `gfx/Input` honouring
  `down` / `up` / `press` / `quit` verbs at frame-accurate edges

## Dependency direction

- Depends on: controller, dialog, entities, gfx, state, world
- Used by: only `main.cpp`
