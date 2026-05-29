# controller — input, simulation step, scene routing, event wiring

The MVC controller layer (CLAUDE.md §5). Owns the per-frame input ->
simulation -> event-bus dispatch pipeline. `main.cpp` wires this to a
View; nothing else in the bucket touches raylib directly.

## Files

- `GameController.h` — per-frame update + event wiring
- `InputHandler.h` — translates raw `gfx/Input` into game intents
- `SceneRouter.h` — title / select / play / ending routing
- `EventBus.h` — `shared_mutex`-guarded event publisher (BUGLEDGER H1)
- `EventWiring.h` — controlled-lifetime subscribers per scene
- `GameObjectFactory.h` — spawn helpers (decouples wiring from `new`)
- `GameObjectQueries.h` — read-only spatial queries over the GameObject
  list (without exposing `World`'s internals)

## Dependency direction

- Depends on: entities, gfx, state, ui, world (headers); plus dialog,
  quest, vendor in src/controller/
- Used by: entities, harness, quest, ui, vendor, world
