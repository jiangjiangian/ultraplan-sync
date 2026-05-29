# tests/controller — input, scene routing, event bus, factory

## Files

- `test_input_handler.cpp` — input -> intent mapping
- `test_scene_router.cpp` — title / select / play / ending routing
- `test_eventbus.cpp` — publish + subscribe core
- `test_eventbus_isolation.cpp` — subscriber lifetime isolation
- `test_eventbus_scoped.cpp` — scoped subscriber RAII (BUGLEDGER H1)
- `test_factory.cpp` — `GameObjectFactory` spawn helpers
- `test_i6_interact_reach.cpp` — interact-reach radius
- `test_i35_interact_vendor.cpp` — interact -> vendor open path

## Dependency direction

- Depends on: dialog, entities, gfx, harness, quest, state, ui, world
- Used by: nothing (test leaf)
