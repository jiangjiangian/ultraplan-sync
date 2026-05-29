# src/quest — per-chapter quest scripts

Quest controllers for chapters 2-4 and the shared `ChapterGate` /
`ChapterVendors` impls. Chapter 1 is the intro and has no script.
Pickup tables (`ChapterPickups`, `ChapterQuestItems`, `ChapterSpawns`)
are header-only constexpr roster tables.

## Files

- `Chapter2Quest.cpp` — midterms quest (umbrella exchange)
- `Chapter3Quest.cpp` — sports day quest
- `Chapter4Quest.cpp` — finals quest (multi-path)
- `ChapterGate.cpp` — chapter completion predicates
- `ChapterVendors.cpp` — vendor spawn impl

## Dependency direction

- Depends on: controller, dialog, entities, gfx, state, ui, vendor
- Used by: controller, dialog, ui, vendor, world
