# Audit — docs/superpowers/plans/2026-05-11-multi-worktree-phases-roadmap.md

**Overview (≤3 sentences):** This 2026-05-11 plan proposes 5 parallel "waves" of `Agent`-dispatched isolated git worktrees to finish Assignment 5 before a 2026-05-12 deadline. The Cycle-6 environment incident (BUGLEDGER lines 688–701, CHANGELOG 169–171) BARRED parallel *worktree* agents — current policy is single-writer sequential (Cycles 7 & 8 enforced this; CHANGELOG 80, 73). Most code-level deliverables (GoF State, NPC, sprite wiring, money HUD, title screen, personas, pause menu, AI-mention pre-commit grep, raylib.h gating, end-of-frame sweep, `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`) shipped via single-writer in Cycles 1–8; only the "parallel-via-Agent" dispatch model itself, the rubric/GoF preamble, the W2 integration commit and the Wave-5 polish remain conflicted/unimplemented.

## Per-element annotations

- **Date / Deadline / HEAD pointer `3757d14`** — header metadata.
  - **[是否實作?]** N/A — pure document header. Current HEAD is `b33db2b` (Cycle 8, CHANGELOG:8).
  - **[邏輯衝突?]** Stale-doc-only — `3757d14` predates the eight gated cycles since (no longer reachable as "current main HEAD").

- **Rubric status 7/7 + GoF status 3/4 (State missing)** — situation report claim.
  - **[是否實作?]** Yes — State pattern present: `include/SemesterStateMachine.h:1`, `src/SemesterStateMachine.cpp:1`, `include/SemesterState.h:1`, plus 5 concrete chapter states (`Chapter1AddDrop.h`, `InterludeMarket.h`, `Chapter2Midterms.h`, `Chapter3SportsDay.h`, `Chapter4Finals.h`).
  - **[邏輯衝突?]** Stale-doc-only — "State missing" no longer true; the gap was closed.

- **Strategy: parallel waves via `Agent isolation:"worktree"`** — dispatch model.
  - **[是否實作?]** No — Cycle-6 incident (BUGLEDGER:688–701) explicitly mandates "no further parallel *worktree* agents in this environment"; Cycles 7 & 8 ran single-writer sequential (CHANGELOG:80 "single-writer, sequential specialists"; CHANGELOG:73 "single-writer throughout (no parallel agents, no worktrees — per the Cycle-6 environment incident)"). Stale `.claude/worktrees/agent-a7cf550937762ff24/` shell remains on disk but unused.
  - **[邏輯衝突?]** Real conflict — directly contradicts the current operating policy.

- **W1A · SemesterStateMachine (GoF State) — 9 new files** — Wave 1 worktree A.
  - **[是否實作?]** Yes — all 9 files ship: `include/SemesterState.h`, `include/SemesterStateMachine.h`, `include/Chapter1AddDrop.h`, `include/InterludeMarket.h`, `include/Chapter2Midterms.h`, `include/Chapter3SportsDay.h`, `include/Chapter4Finals.h`, `src/SemesterStateMachine.cpp`, `tests/test_state_machine.cpp`.
  - **[邏輯衝突?]** Stale-doc-only — landed single-writer, not via the proposed W1A worktree dispatch.

- **W1B · NPC class skeleton (NPC.h/.cpp/test)** — Wave 1 worktree B.
  - **[是否實作?]** Yes — `include/NPC.h:14` (`class NPC : public Character`), `src/NPC.cpp`, `tests/test_npc.cpp`. Includes wander/seed extension beyond the brief.
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 2 · sequential integration (ObjectType::NPC enum, ChapterChanged event, machine.Transition on EnteredBuilding(井塘樓), NPC spawn)** — integration commit.
  - **[是否實作?]** Partial — `EventType::EnteredBuilding` exists (`include/EventBus.h:14`); `SemesterStateMachine` is wired through `ChapterGate.cpp:9` and `EndingGate.cpp:19` (chapter gating via ChapterGate, not via the literal `EnteredBuilding(井塘樓)` trigger). No `ObjectType::NPC` enum in `include/GameObjectFactory.h` (factory class shell only, `include/GameObjectFactory.h:27`); no `EventType::ChapterChanged` (`grep` empty). NPCs spawn in `World`, not via the factory.
  - **[邏輯衝突?]** Stale-doc-only — actual integration is more decoupled (ChapterGate + 7-cycle quest spine) than the plan's "井塘樓 entry triggers Transition(InterludeMarket)".

- **Wave 3A · Asset gen (player 4-dir×3 frames, 4 umbrellas, 5 NPC sprites)** — background asset generation.
  - **[是否實作?]** No — `resources/assets/sprites/player_*.png`, `umbrella_*.png`, `npc_*.png` absent (`ls` errored). CLAUDE.md §1 notes "fresh clone has an empty `resources/`" by design.
  - **[邏輯衝突?]** Stale-doc-only — the project deliberately ships without bespoke assets (CLAUDE.md §5 "Don't version new assets into `resources/`"); rendering falls back to atlas/silhouettes (Cycle-7 R9 `8bc9c9f` data-driven `UmbrellaStyle`).

- **Wave 3B · Sprite wiring (Player::Draw, TransparentUmbrella render-event, HUD umbrella icons)** — sequential render wiring.
  - **[是否實作?]** Partial — Player sprite + tint mapping present (`include/Player.h:86 SetTint`; Cycle-6 UI cycle CHANGELOG:622–658); 4 distinct umbrellas via data-driven `UmbrellaStyle` (CHANGELOG:117, R9 fix `8bc9c9f`); HUD has rain%, money, building (Cycles 3/6/7). No `TransparentUmbrella` family exists in the codebase (grep empty in Player); the GDD red line "Player not include TransparentUmbrella header" is vacuously held.
  - **[邏輯衝突?]** Stale-doc-only — the plan references a `TransparentUmbrella` class hierarchy that the shipped engine does not have.

- **Wave 4A · CH1 content draft (3–5 NPC dialog markdown drafts)** — narrative drafts.
  - **[是否實作?]** Yes — `docs/content/chapter1.md` (+ ch2/3/4) carry the full 4-chapter spine loaded via `DialogLoader.cpp` (CLAUDE.md §6); Cycle-8 F2 (CHANGELOG:8–34) most recent edit.
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 4B · CH1 wiring (3 NPCs at named buildings; Chapter1AddDrop::Enter() loads dialog or inline constexpr)** — runtime wiring.
  - **[是否實作?]** Partial — dialog is runtime-loaded by `DialogLoader.cpp` per CLAUDE.md §2.3 (correctly, not via `Chapter1AddDrop::Enter()` inline constexpr — that would violate "No codegen" §5). NPCs spawn in `World` and are routed by `DialogOpener.cpp` (CHANGELOG references throughout Cycles 1–8).
  - **[邏輯衝突?]** Real conflict (mild) — the proposed `inline constexpr std::array` ingestion path collides with CLAUDE.md §5 "No codegen. Dialogue is runtime-loaded".

- **Wave 4C · CH2–4 stubs (1-line placeholder per chapter)** — minimal demo content.
  - **[是否實作?]** Yes — superseded; CH2/3/4 carry full content (Cycle 7 R5 rain audit traced "Ch2 學霸 + 圖書館管理員; Ch3 香腸→大聲公→學姊; Ch4 助教 finale", BUGLEDGER:792–797).
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 5 · Dialog box / HUD frame UI overlays** — final polish item.
  - **[是否實作?]** Yes — dialog wrap+paginate (`include/DialogLayout.h`, B4 fix CHANGELOG:417–432); HUD panel + rain readout + vignette (V2/V3 Cycle 3, CHANGELOG:315–335); money HUD (UI cycle CHANGELOG:625).
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 5 · Save state JSON dump on quit (resources/save/ gitignored)** — persistence.
  - **[是否實作?]** No — `ls resources/save/` absent; no quit-save in `src/`. Only `state.jsonl` produced by the autoplay harness (CLAUDE.md §4).
  - **[邏輯衝突?]** Stale-doc-only — assignment treats `state.jsonl` as state-truth (CLAUDE.md §1); save-on-quit was never picked up.

- **Wave 5 · README polish + UML.md → PNG export** — submission artifacts.
  - **[是否實作?]** Partial — `README.md` and `UML.md` both present at repo root; no PNG export of UML observed.
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 5 · Final architecture-reviewer + gof-pattern-auditor + codex:adversarial-review triple pass** — review pipeline.
  - **[是否實作?]** Partial — cycle gates run the equivalent via `cpp-architecture-reviewer` + `dialog_lint` + `playtest.sh` (CLAUDE.md §7); the named `gof-pattern-auditor` and `codex:adversarial-review` sub-agents are not part of the current single-writer flow.
  - **[邏輯衝突?]** Stale-doc-only.

- **Wave 5 · User decides: push to origin/main** — release gate.
  - **[是否實作?]** Yes — CLAUDE.md §5 "Never push … `git push` is the human's"; Cycle-8 ends at `b33db2b == origin` (CHANGELOG:66).
  - **[邏輯衝突?]** No.

- **Dispatch rule 1: AI-mention pre-commit grep** — pre-commit hook.
  - **[是否實作?]** No — no `.git/hooks/pre-commit` shipped; CLAUDE.md §5 enforces "Forbidden strings" as a contract reviewed manually. `.gitignore` ignores `.claude/` to keep agent text local.
  - **[邏輯衝突?]** Stale-doc-only — policy is enforced by humans/agents, not by a hook.

- **Dispatch rule 2: gate = `cmake --build` + `ctest` + 5 s smoke; never `rm -rf build`** — convergence bar.
  - **[是否實作?]** Yes — CLAUDE.md §5 "Don't `rm -rf build` (trust incremental)"; every cycle gate logs build/test/playtest (e.g. CHANGELOG:65–76 Cycle-8 gate).
  - **[邏輯衝突?]** No.

- **Dispatch rule 3: three red lines (no TransparentUmbrella include in Player; no DrawText/DrawTexture in Item/Umbrella family; `objects.erase` in end-of-frame sweep)** — red lines.
  - **[是否實作?]** Partial/Yes — Player has no `TransparentUmbrella` include (`grep` empty; the family doesn't exist anyway); umbrella `src/*Umbrella.cpp` grep for `DrawText`/`DrawTexture` returned empty (render-only contract held); end-of-frame sweep lives at `src/World.cpp:178` `objects_.erase(` and `src/GameController.cpp:531`.
  - **[邏輯衝突?]** Stale-doc-only on `TransparentUmbrella` (the class never existed in the shipped engine).

- **Dispatch rule 4: `raylib.h` only inside `include/gfx/*.h`** — header isolation.
  - **[是否實作?]** Partial — most `raylib.h` includes are under `include/gfx/` (Font.h, Window.h, Texture.h, Renderer.h, Input.h, Time.h, Key.h, DrawScope.h, CameraScope.h, MaskLoader.h, TextBuilder.h); `include/World.h`, `include/InventoryView.h`, `src/Harness.cpp`, `src/ScriptInput.cpp`, `src/MessageView.cpp` also pull it in.
  - **[邏輯衝突?]** Real conflict (mild) — the rule is broader than current practice; non-gfx headers/sources include raylib.h. CLAUDE.md §5 enforces MVC purity but not this exact "raylib.h ⊂ gfx/" gate.

- **Dispatch rule 5: `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` on every fresh worktree** — configure flag.
  - **[是否實作?]** Yes — `CMakeLists.txt:22-24` auto-sets it when not defined; `.claude/settings.json:25` SessionStart hook passes the flag; CLAUDE.md §1 documents it.
  - **[邏輯衝突?]** No.

- **Dispatch rule 6: no `git push` from any session** — push policy.
  - **[是否實作?]** Yes — CLAUDE.md §5 "Never push … `git push` is the human's".
  - **[邏輯衝突?]** No.

- **Dispatch rule 7: post-merge trio of reviewers in parallel** — review gating.
  - **[是否實作?]** Partial — Cycle gates run a comparable trio (`cpp-architecture-reviewer`, `playtest-bughunter`, `dialog_lint`) but sequentially in the current single-writer policy.
  - **[邏輯衝突?]** Real conflict — "in parallel" is barred by the Cycle-6 incident; reviewers run sequentially now.

## Summary

- **Elements audited:** 23
- **Logical conflicts (Real):** 4 — parallel-worktree dispatch model; Wave-4B inline constexpr ingestion vs §5 no-codegen; rule-4 `raylib.h ⊂ gfx/` vs current scattering; rule-7 parallel reviewer trio
- **Logical conflicts (Stale-doc-only):** 13
- **No conflict:** 5
- **N/A:** 1 (document header date)
- **Yes implemented:** 10; **Partial:** 7; **No:** 3; **N/A:** 1; **Stale framing of "GoF State missing":** superseded

The plan's *deliverables* mostly shipped (single-writer, Cycles 1–8). The plan's *dispatch model* (parallel worktrees + Agent isolation) is structurally incompatible with the post-Cycle-6 single-writer policy and should be treated as historical.
