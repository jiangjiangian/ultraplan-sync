# Remaining Phases — Multi-Worktree Roadmap

**Date:** 2026-05-11
**Deadline:** 2026-05-12 (assignment 5 GitHub Classroom)
**Current main HEAD:** `3757d14 feat: world bounds clamp + 27-building entry triggers`
**Rubric status:** 7/7 satisfied (per gof-pattern-auditor)
**GoF status:** 3/4 — Factory Method / Observer / Template Method done; **State missing**

---

## Strategy

Future work runs as **parallel waves**: each wave kicks off N independent worktrees + agents, then converges with sequential merges. Independence is engineered into the file-touch boundaries so worktrees do not conflict.

Within a wave, agents are dispatched via `Agent` tool with `isolation: "worktree"` (creates a temporary git worktree) running in background. Each agent gets a self-contained plan in its prompt.

---

## Wave 1 — Fill the GoF gap + NPC scaffolding  ✦ **PARALLEL, FROM MAIN @ 3757d14**

Both worktrees touch **only new files**. Zero conflict surface.

### W1A · SemesterStateMachine (GoF State)
- New files: `include/SemesterState.h`, `include/SemesterStateMachine.h`, `include/Chapter1AddDrop.h`, `include/InterludeMarket.h`, `include/Chapter2Midterms.h`, `include/Chapter3SportsDay.h`, `include/Chapter4Finals.h`, `src/SemesterStateMachine.cpp`, `tests/test_state_machine.cpp`
- Pattern: GoF State — abstract `IChapterState` interface (Enter / Update / Exit / Name), 5 concrete chapter states, 3 ending sentinel states in an enum. `SemesterStateMachine` owns the current `IChapterState*` + handles `Transition(SemesterState)`.
- No changes to: `EventBus.h`, `GameObjectFactory.h`, `main.cpp`. Wave 2 wires.

### W1B · NPC class skeleton
- New files: `include/NPC.h`, `src/NPC.cpp`, `tests/test_npc.cpp`
- NPC inherits `Character`; holds `std::vector<std::string> dialogLines`, `size_t currentLineIndex`, `bool isQuestGiver`. `Interact()` advances dialog index; `Draw()` draws a placeholder rect; `Update()` is no-op.
- No changes to: `EventBus.h`, `GameObjectFactory.h`, `main.cpp`. Wave 2 wires.

---

## Wave 2 — Integration  ✦ **SEQUENTIAL, AFTER W1A + W1B MERGED**

Main session does merging + integration (these need cross-file edits that would conflict if parallelised).

- Merge W1A → main (squash)
- Merge W1B → main (squash)
- **W2 integration commit on main:**
  - Add `ObjectType::NPC` to `GameObjectFactory.h` enum + factory function dispatch
  - Add an `EventType::ChapterChanged` (or re-use `ShowMessage`) to broadcast state transitions
  - In `main.cpp`: construct `SemesterStateMachine`; on `EnteredBuilding(井塘樓)` event (or another suitable trigger) call `machine.Transition(InterludeMarket)`; spawn 1 NPC near `井塘樓` for visual confirmation
- Verification gate: build clean, full test pass, 5s smoke

---

## Wave 3 — Visual polish (assets + sprite wiring)  ✦ **PARTIALLY PARALLEL**

Assets are independent — agents (or `nanobanana` direct calls) can generate concurrently. Sprite wiring is a single sequential commit on main.

### W3A · Asset gen (background, multiple files)
- Player 4-dir × 3 frames → `resources/assets/sprites/player_<dir>_<frame>.png`
- 4 transparent umbrella icons → `resources/assets/sprites/umbrella_<variant>.png`
- 5 NPC sprites → `resources/assets/sprites/npc_<role>.png`

(Assets are untracked anyway; nothing to commit. Generation cost is the only constraint.)

### W3B · Sprite wiring on main (one PR)
- `Player::Draw` loads / draws sprite Texture using direction frame
- `TransparentUmbrella::Draw` (via `RenderRequested` event payload) carries a tint + sprite name
- HUD frame Texture for the umbrella icons in the inventory line

---

## Wave 4 — Content (CH1 first, CH2–4 if time)  ✦ **PARALLEL DRAFTS, SEQUENTIAL WIRING**

### W4A · CH1 content draft
- Markdown drafts of 3–5 NPC dialog scripts for `Chapter1_AddDrop`
- Each NPC gets lines indexed by interaction count

### W4B · CH1 wiring on main
- Spawn the 3 CH1 NPCs at named building locations
- Wire `Chapter1AddDrop::Enter()` to load NPC dialog from the markdown drafts (or `inline constexpr std::array` like `Buildings.h`)

### W4C · CH2–4 stubs
- One line of placeholder dialog per chapter so the State pattern transition demo flows end-to-end

---

## Wave 5 — Final polish  ✦ **SINGLE-PASS ON MAIN**

- Dialog box / HUD frame UI overlays
- Save state JSON dump on quit (placeholder file under `resources/save/` — gitignored)
- README polish + UML.md export to PNG for grading submission
- Final `architecture-reviewer` + `gof-pattern-auditor` + `codex:adversarial-review` triple pass
- User decides: push to `origin/main`

---

## Dispatch & verification rules (binding for every wave)

1. Every commit goes through the AI-mention pre-commit grep (`claude|codex|superpowers|nanobanana|gemini|anthropic|ai agent|CLAUDE\.md|AGENTS\.md`).
2. Every wave ends with `cmake --build build` + `ctest` + 5 s `./build/OOP_Raylib_Lab` smoke. Incremental — **never** `rm -rf build`.
3. Three architectural red lines hold across all phases:
   - Player does not include any concrete `TransparentUmbrella` subclass header.
   - Item / Umbrella family does not call `DrawText` or `DrawTexture`.
   - Main loop's `objects.erase` stays in the end-of-frame sweep slot, after the `DrawScope` block closes.
4. `raylib.h` only inside `include/gfx/*.h`.
5. Configure flag: `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` on every fresh worktree.
6. No `git push` from any session. User pushes manually.
7. Each merged wave is reviewed by the trio (`architecture-reviewer`, `gof-pattern-auditor`, `codex:adversarial-review`) in parallel before the next wave starts.
