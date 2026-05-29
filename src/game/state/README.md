# src/state — semester state machine + ending gate impls

`SemesterStateMachine` owns the active state and runs the transitions
declared in the chapter/interlude headers. `EndingGate` reads karma,
inventory, and flags to fire endings A/B/C.

The chapter and interlude states themselves
(`Chapter1AddDrop`/`Chapter2Midterms`/...) are header-only — their
enter/exit/update bodies are inline. If they grow non-trivial impls,
add a matching `.cpp` here; CMakeLists.txt's `GLOB_RECURSE` picks them
up at next configure.

## Files

- `SemesterStateMachine.cpp` — state lifecycle + dispatch
- `EndingGate.cpp` — endings A/B/C decision

## Dependency direction

- Depends on: dialog, entities, ui
- Used by: controller, harness, quest, ui, world
