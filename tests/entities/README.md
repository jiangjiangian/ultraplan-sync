# tests/entities — Player, NPC, items, pickups, umbrellas

Behaviour tests for the GameObject hierarchy.

## Files

- `test_player.cpp`, `test_player_core.cpp` — Player update + interact
- `test_npc.cpp` — NPC patrol + interact
- `test_npc_loaddialog.cpp` — NPC dialog loading
- `test_consumable.cpp` — consumable effects + one-shot guard
- `test_cashpickup.cpp` — money pickup
- `test_quest_pickup.cpp`, `test_quest_pickup_render.cpp` — flag pickup
  + render
- `test_umbrella_render.cpp` — umbrella sprite render
- `test_cursed_penalty_scale.cpp` — CursedUmbrella karma penalty
- `test_rain_survival.cpp` — rain-meter consumable interaction

## Dependency direction

- Depends on: controller, dialog, gfx, state, world (test fixtures)
- Used by: nothing (test leaf)
