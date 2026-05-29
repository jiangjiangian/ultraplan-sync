# tests/harness — scripted input driver tests

The autoplay harness itself (`Harness.cpp`) is tested indirectly via
the playtest gate (CLAUDE.md §4); this bucket tests the script-input
driver which is the harness's deterministic actuation source.

## Files

- `test_scriptinput.cpp` — `down` / `up` / `press` / `quit` verbs
  at frame-accurate edges
- `test_scriptinput_classic_move.cpp` — held-key movement
- `test_scriptinput_plan.cpp` — plan-style script (higher-level verbs)

## Dependency direction

- Depends on: controller, dialog, entities, gfx, world
- Used by: nothing (test leaf)
