# Audit — docs/content/ending_a.md

**Overview (≤3):** (1) The Ending-A *trigger* gate is wired exactly per CLAUDE.md §3 baseline at `src/EndingGate.cpp:27-31` (`karma>80 && Flag_HasTrueUmbrella && Flag_ConsoledTA`). (2) The *post-trigger payoff* — 5 NPC callbacks (學長/學霸/助教/阿姨/苦主), opening/middle/epilogue 字卡 (≈14 lines), monologue, BGM unlock, save flag, replay-lock — is almost entirely UNimplemented; `EndingView::DrawEndingCard` (`src/EndingView.cpp:67-93`) renders a single caption and `kEndingA` NPC roster is empty (`include/ChapterSpawns.h:93`, TODO(S5e)). (3) The `ending_sequence_order` (9-step sequencer) and `npc_spawn_conditions` (5 gating flags) section is design-doc-only — there is no scripted ending sequencer in code.

## Per-element annotations

- **metadata.trigger_condition (karma>80 + TrueUmbrella + 體諒)** — three-way Ending-A gate
  - [是否實作?] **Yes** — `src/EndingGate.cpp:27-31`; tested `tests/test_ending_gate.cpp` (per CHANGELOG L1).
  - [邏輯衝突?] **No** — matches CLAUDE.md §3 baseline verbatim.

- **metadata.ta_branch: 體諒 (Ch4 d-segment)** — Flag_ConsoledTA setter is Ch4 助教 (d) choice
  - [是否實作?] **Yes** — `src/DialogOpener.cpp:298` ("體諒助教的辛勞", `Flag_ConsoledTA, +15`).
  - [邏輯衝突?] **No**.

- **metadata.main_quests_cleared: Ch1+2+3+4** — all-chapter clear precondition
  - [是否實作?] **Partial** — implicit (Flag_ConsoledTA only set after reaching Ch4 finale; spine progression gates each chapter), not an explicit AND-check in `EndingGate`.
  - [邏輯衝突?] **No** (mechanically equivalent via flag-chain).

- **metadata.state_entry: Ending_A** — semester transition
  - [是否實作?] **Yes** — `src/EndingGate.cpp:30` `semester.Transition(SemesterState::Ending_A)`; `include/SemesterState.h:13`.
  - [邏輯衝突?] **No**.

- **metadata.cannot_replay / replay_lock: true** — once entered, save read-only
  - [是否實作?] **No** — no save system or replay-lock logic exists in `src/`/`include/` (grep clean for `cannot_replay`/`replay_lock`/save). Pause menu offers 重新開始 unconditionally (CHANGELOG UI Cycle).
  - [邏輯衝突?] **No** (stale-doc-only; never gated by red lines).

- **metadata.save_flag_written: Flag_EndingA_True** — persistent ending flag
  - [是否實作?] **No** — no `SetFlag("Flag_EndingA_True")` anywhere in `src/`/`include/`; not in `Harness.cpp::KnownFlags()` whitelist.
  - [邏輯衝突?] **No** (stale-doc-only).

- **metadata.bgm_unlock: ending_a_theme** — main-menu BGM unlock on flag
  - [是否實作?] **No** — no BGM/audio system; no flag-gated unlocks anywhere.
  - [邏輯衝突?] **No** (stale-doc-only).

- **二、開場字卡 (4 字卡)** — opening narration cards
  - [是否實作?] **No** — `EndingView::DrawEndingCard` renders ONE caption only (`「雨過天晴，傘還在你手上。」`, `EndingView.cpp:33`); no sequencer plays the 4 開場 cards.
  - [邏輯衝突?] **No** (V3 in CHANGELOG explicitly authored only the single caption).

- **三、五 NPC Callback 收束 (學長/學霸/助教/阿姨/苦主)** — 5 post-victory dialogues
  - [是否實作?] **No** — `include/ChapterSpawns.h:93` `kEndingA` declared `static const std::vector<NpcSpawn>` with no initializer = empty roster; comment `// TODO(S5e): chapter roster`. None of the 5 NPCs spawn in `Ending_A`. `DialogSource.cpp:43` does pin `ending_a.md`, but no NPC exists to trigger `Entries()` lookup.
  - [邏輯衝突?] **No** — INTENTIONAL Cycle-3+ scope: V3 narrowed to single-card; CHANGELOG never claimed callbacks. Stale-doc-only.

- **四、主角內心獨白 (6 字卡)** — protagonist monologue cards
  - [是否實作?] **No** — same reason as 開場字卡; no card-sequencer.
  - [邏輯衝突?] **No** (stale-doc-only).

- **五、結局字卡 Epilogue (10 字卡)** — multi-NPC future flash-forward
  - [是否實作?] **No** — same as above.
  - [邏輯衝突?] **No** (stale-doc-only).

- **六、製作組提示 (ending_sequence_order, npc_spawn_conditions, dialog_render)** — 9-step sequencer + 5-flag spawn gate spec
  - [是否實作?] **No** — no `ending_sequence_order` machine; `npc_spawn_conditions` 5 flags (Flag_HelpedSenior/PromisedVictim/HelpedTA_Ch1/hasItem(TrueUmbrella)/karma>80) are mostly wired individually (`DialogOpener.cpp` various lines) but never AND-gated to control ending NPC spawning. `dialog_render.max_line_width_chars: 28` is the actual `DialogLayout` rule (`include/DialogLayout.h`, CHANGELOG B4).
  - [邏輯衝突?] **No** (implementation-notes are advisory; the 28-cell width is correctly enforced).

## Summary

- Implemented: **4** (trigger gate, ta_branch setter, state_entry, 28-cell width rule)
- Partial: **1** (main_quests_cleared as implicit flag-chain)
- Not-implemented: **9** (replay_lock, save_flag, bgm_unlock, 開場字卡, 5 NPC callbacks, 主角獨白, epilogue 字卡, ending_sequence_order, npc_spawn_conditions AND-gate)
- Conflicts: **0** REAL (all gaps are stale-doc / INTENTIONAL Cycle-3+ scope per CHANGELOG V3 — only the single caption was authored)
- Stale-doc-only: **9** (all non-trigger payoff content)
