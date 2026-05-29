# quest — chapter-scoped quest scripts and spawn tables

Per-chapter quest objectives, gate transitions, and the tables that
populate world rosters (NPC spawns, pickups, vendors, quest items). Acts
on the GameObject world but does not own it; the `World` calls into
these to materialise a chapter.

## Files

- `QuestObjective.h` — pure-data quest description
- `Chapter2Quest.h`, `Chapter3Quest.h`, `Chapter4Quest.h` — per-chapter
  quest controllers; chapter 1 has no script (intro)
- `ChapterGate.h` — chapter completion gate predicates
- `ChapterPickups.h` — pickup roster table per chapter
- `ChapterQuestItems.h` — quest-flag-bearing items per chapter
- `ChapterSpawns.h` — NPC spawn table per chapter
- `ChapterVendors.h` — vendor roster + locations per chapter
- `NpcSpawns.h` — shared NPC spawn primitives
- `PipoyaRoster.h` — Pipoya sprite-sheet roster used by chapter spawns

## Dependency direction

- Depends on: entities, gfx, state, vendor (headers); plus controller,
  dialog, ui in src/quest/ wiring
- Used by: controller, dialog, ui, vendor, world
