# src/world — World container, terrain mask loader, building tracker

Heavy-weight world wiring. `World.cpp` owns the GameObject vector and
applies the chapter roster; `BuildingTracker.cpp` watches the player's
tile to drive building-scoped dialog; `TerrainMask.cpp` loads the
greyscale mask layered with rect obstacles.

## Files

- `World.cpp` — `loadChapter`, deferred deletion, rain meter step
- `BuildingTracker.cpp` — building-enter/leave events
- `TerrainMask.cpp` — `CollisionMask` impl over greyscale png

## Dependency direction

- Depends on: controller, entities, gfx, quest, vendor
- Used by: controller, ui, harness (via headers)
