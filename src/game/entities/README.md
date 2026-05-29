# src/entities — implementations of the GameObject hierarchy

One `.cpp` per actor whose header declares non-trivial methods.
`GameObject`, `Character`, `Item`, and `ConsumableItem` are
header-only abstractions so they have no `.cpp` here.

## Files

- `Player.cpp` — movement, interact, karma/money mutations
- `NPC.cpp` — patrol + dialog opening
- `CashPickup.cpp` — collect + animate
- `QuestFlagPickup.cpp` — sets `Flag_*` on collect
- `HotPack.cpp`, `EnergyDrink.cpp`, `WaterproofSpray.cpp` — consume effects
- `TransparentUmbrella.cpp` — starter behaviour
- `TrueUmbrella.cpp` — true-ending requirement, `beClaimed` guard
- `FragileUmbrella.cpp` — N-step rain countdown
- `CursedUmbrella.cpp` — karma penalty scale
- `ProfessorTrapUmbrella.cpp` — chapter-2 trap variant

## Dependency direction

- Depends on: controller, dialog, gfx, world
- Used by: every gameplay bucket (controller, quest, ui, world, ...)
