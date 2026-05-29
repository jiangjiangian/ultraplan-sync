# entities — GameObject hierarchy (Player, NPC, Items, Umbrellas)

All gameplay actors that participate in the world's GameObject list.
Each derives (directly or indirectly) from `GameObject` and implements
the update/draw/interact contract.

## Files

- `GameObject.h` — base class with id, pos, AABB, active flag
- `Character.h` — abstract walker (Player + NPC share this)
- `Player.h` — playable character, karma/money owner
- `NPC.h` — non-player walker, dialog source
- `Item.h` — abstract pickable
- `ConsumableItem.h` — base for one-shot consumables
- `CashPickup.h` — money pickup
- `QuestFlagPickup.h` — sets a `Flag_*` when picked up
- `HotPack.h`, `EnergyDrink.h`, `WaterproofSpray.h` — consumables
- `TransparentUmbrella.h` — starter umbrella
- `TrueUmbrella.h` — true ending requirement
- `FragileUmbrella.h` — breaks after N steps in rain
- `CursedUmbrella.h` — karma penalty, ending B trigger
- `ProfessorTrapUmbrella.h` — chapter-2 trap variant

## Dependency direction

- Depends on: gfx, state, world (header-level)
- Used by: controller, dialog, harness, quest, state, ui, vendor, world
