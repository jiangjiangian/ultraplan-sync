# world — physical world: buildings, obstacles, collision, World root

Holds the static playfield (buildings + obstacle layout), the collision
mask layering, physics step, and the `World` container that owns the
GameObject vector and chapter-scoped roster.

## Files

- `World.h` — root container; owns `objects_`, chapter, rain meter
- `WorldConfig.h` — compile-time map dims and tile sizes
- `Buildings.h` — campus building rectangles with names
- `Obstacles.h` — static collision rects (trees, fences)
- `BuildingTracker.h` — which building the player is in (for dialog)
- `CollisionMask.h` — greyscale-tile mask layered with rects
- `Physics.h` — AABB step + slide along masks

## Dependency direction

- Depends on: dialog, entities, gfx, state, ui (headers); plus
  controller, quest, vendor in `src/world/` for wiring
- Used by: controller, entities, gfx (`MaskLoader`), harness, ui
