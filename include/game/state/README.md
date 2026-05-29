# state — semester state machine + chapter/interlude states + ending gate

The `SemesterStateMachine` orchestrates chapters and interludes; each
state file is a pure-data + transition rule pair. `EndingGate` reads
karma + flags + inventory to decide which ending fires.

## Files

- `SemesterState.h` — abstract state interface (enter/exit/update)
- `SemesterStateMachine.h` — owns active state, runs transitions
- `Chapter1AddDrop.h` — Chapter 1: add/drop period intro
- `Chapter2Midterms.h` — Chapter 2: midterms storm
- `Chapter3SportsDay.h` — Chapter 3: sports day
- `Chapter4Finals.h` — Chapter 4: finals
- `InterludeMarket.h` — between-chapter market visit
- `InterludeExit.h` — interlude exit state
- `InterludeExitMarker.h` — visual marker for the exit
- `EndingGate.h` — endings A/B/C decision logic

## Dependency direction

- Depends on: gfx (headers); plus dialog, entities, ui in src/state/
- Used by: controller, dialog, entities, harness, quest, ui, world
