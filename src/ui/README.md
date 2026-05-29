# src/ui — implementations of HUDs, views, and screens

The render layer. `View.cpp` is the top-level scene render entry that
composes the other views. Smaller HUD elements (`HudSlot`, `RainHud`,
`ChapterToast`, `QuestGiverIndicator`, `GameHelp`, `ReducedMotion`) are
header-only.

## Files

- `View.cpp` — top-level scene render entry
- `TitleScreen.cpp` — start screen draw
- `CharacterSelect.cpp` — sprite picker draw + input feedback
- `EndingView.cpp` — A/B/C ending card draw
- `InventoryView.cpp` — inventory grid draw
- `MessageView.cpp` — transient HUD message queue + render

## Dependency direction

- Depends on: controller, dialog, entities, gfx, quest, state, world
- Used by: controller, quest, state, world
