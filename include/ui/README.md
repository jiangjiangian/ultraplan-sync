# ui — HUD, views, screens, accessibility

Everything the player sees outside the world tile draw: title screen,
character select, HUDs, inventory, ending card, accessibility helpers.
Pure-render layer; nothing here mutates world state.

## Files

- `View.h` — top-level scene render entry
- `TitleScreen.h` — start screen
- `CharacterSelect.h` — sprite picker
- `EndingView.h` — A/B/C ending card
- `InventoryView.h` — inventory grid
- `MessageView.h` — transient HUD messages
- `RainHud.h` — rain meter HUD
- `HudSlot.h` — fixed HUD positions enum
- `ChapterToast.h` — chapter-change toast (separate HUD channel)
- `QuestGiverIndicator.h` — `!` over quest givers
- `GameHelp.h` — pause-menu help text
- `ReducedMotion.h` — a11y toggle for reduced animation

## Dependency direction

- Depends on: controller, gfx, state (headers); plus dialog, entities,
  quest, world in src/ui/
- Used by: controller, quest, state, world
