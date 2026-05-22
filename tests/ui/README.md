# tests/ui — HUDs, views, screens, accessibility

## Files

- `test_inventory_view.cpp` — inventory grid
- `test_message_view.cpp` — HUD message queue
- `test_ending_card_render.cpp` — A/B/C ending card
- `test_quest_giver_indicator.cpp` — `!` over quest givers
- `test_rain_hud_redundant.cpp` — RainHud redundancy invariant
- `test_two_hud_channels.cpp` — chapter-toast vs message-view channels
- `test_karma_toast.cpp` — karma toast
- `test_hud_reset.cpp` — HUD restart safety
- `test_chapter_toast.cpp` (none; covered by `test_two_hud_channels`)
- `test_font_ui_glyphs.cpp` — CJK font atlas glyph coverage
- `test_menu_help.cpp` — pause-menu help
- `test_press_latch.cpp` — title/select/help keypress-leak latch
- `test_pause_menu_toggle.cpp` — pause toggle UI
- `test_reduced_motion.cpp` — a11y reduced-motion toggle
- `test_large_targets.cpp` — a11y large target hitboxes
- `test_accessibility_contrast.cpp` — a11y color contrast
- `test_restart_safety.cpp` — full-game restart safety

## Dependency direction

- Depends on: controller, entities, gfx, state, world
- Used by: nothing (test leaf)
