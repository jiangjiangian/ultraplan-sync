# src/controller — input + simulation + event wiring

Implements the MVC controller layer (CLAUDE.md §5). Each frame the
`GameController` runs InputHandler -> simulation tick -> event flush,
without touching raylib directly.

## Files

- `GameController.cpp` — per-frame loop body (split from a monolith in
  Cycle 10.P0a per `awsome_cpp.md` §6)
- `InputHandler.cpp` — raw `gfx/Input` -> intent translation
- `SceneRouter.cpp` — title / select / play / ending routing
- `EventBus.cpp` — `shared_mutex`-guarded publisher (BUGLEDGER H1)
- `GameObjectFactory.cpp` — spawn helpers

`GameObjectQueries` and `EventWiring` are header-only utilities.

## Dependency direction

- Depends on: dialog, entities, gfx, quest, state, ui, vendor, world
- Used by: entities, harness, quest, ui, vendor, world
