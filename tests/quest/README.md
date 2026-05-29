# tests/quest — chapter scripts, spines, ripple, gates, economy

Largest test bucket — every chapter has spine + ripple + roster tests
plus shared economy and gate tests.

## Files

- `test_ch{2,3,4}_quest.cpp` — per-chapter quest controller
- `test_ch{2,3,4}_ripple.cpp` — ripple flags seeded by chapter
- `test_ch{1,4}*` — spine reachability + finale
- `test_ch4_routing.cpp`, `test_chapter4_senior_skip.cpp` — finale routing
- `test_chapter{2,3,4}_roster.cpp` — chapter NPC roster
- `test_chapter_spawns.cpp`, `test_chapter_spine.cpp`,
  `test_chapter_transitions.cpp`, `test_chapter_questitems.cpp`,
  `test_chapter_gate.cpp` — shared chapter primitives
- `test_economy_loop.cpp` — money / vendor economy
- `test_ripple_seed_flags.cpp` — ripple-flag whitelist
- `test_spawn_reachability.cpp` — pathable spawn coords
- `test_suit_senior_oneshot.cpp` — one-shot dialog guard
- `test_loadchapter_chapter1.cpp` — `World::loadChapter` chapter 1

## Dependency direction

- Depends on: controller, dialog, entities, gfx, harness, state, ui, world
- Used by: nothing (test leaf)
