# Audit — docs/content/ending_b.md

**Overview (≤3 sentences):**
`ending_b.md` is a pure narrative-presentation document — its dialogue is *not* loaded into any in-game `NPC` (the loader requires `## NPC：<name>` headings (`DialogLoader.cpp:39-42`); `ending_b.md` uses `## 一、…` / `### 西裝學長` section/sub-NPC headings, so substates `### (a)..(d)` are absent and no lines reach `DialogState`). On reaching `Ending_B` the engine only renders the 1-line caption "「你成為了你曾經最討厭的那種人」" via `EndingView.cpp:34-35` (grey-tinted at `EndingView.cpp:61`); every multi-NPC reaction beat, 字卡 sequence and epilogue card written here is presentation-only and currently unrenderable. Trigger conditions match `EndingGate.cpp:47-53` (Cycle-6 L1 extended GDD `Flag_TookCursedUmbrella || karma<0` with `coldFinale`).

## Per-element annotations

- **YAML `state: nccu::SemesterState::Ending_B`** — declares engine state.
  - **[是否實作?]** Yes — `include/SemesterState.h:14`, transition at `src/EndingGate.cpp:51`.
  - **[邏輯衝突?]** No.

- **YAML `trigger_conditions: karma < 0`** — first listed gate.
  - **[是否實作?]** Yes — `EndingGate.cpp:50` (`player.GetKarma() < 0`).
  - **[邏輯衝突?]** No (real conflict). Karma clamp `[-100,100]` starts 50; non-cursed paths floor ≈ −38 (BUGLEDGER F3a), so this clause is a defensive lower bound documented in `遊戲企劃與敘事架構.md` — intentional, not a defect.

- **YAML `trigger_conditions: inventory.hasItem("CursedUmbrella") == true AND player reaches Ch4 endpoint`** — listed gate.
  - **[是否實作?]** Partial — `Flag_TookCursedUmbrella` is the actual gate (`CursedUmbrella.cpp:14`, `EndingGate.cpp:49`), not an inventory query; there is no `hasItem("CursedUmbrella")` API. Gate is `Chapter4_Finals` only (`EndingGate.cpp:21`), so "reaches Ch4 endpoint" is honoured.
  - **[邏輯衝突?]** Yes (minor doc-vs-code drift) — the YAML uses an inventory-query vocabulary the codebase does not implement; semantic equivalent is met via the persistent flag.

- **YAML `trigger_conditions: player picks up named umbrella from rack during Ch4`** — third listed gate.
  - **[是否實作?]** No — no Ch4 named-umbrella-rack item exists in `World.cpp` (only Ch1 `CursedUmbrella` at `{1560,1280}` `World.cpp:32`); Cycle-6 L1 added the **cold-finale** branch (`Flag_TaFinaleChoiceMade && !Flag_ConsoledTA`, `EndingGate.cpp:47-48`) instead.
  - **[邏輯衝突?]** Yes (real conflict) — YAML names a third trigger that the engine does not provide; the engine's actual third trigger (`coldFinale`) is undocumented in this YAML. Intentional Cycle-6 redesign, but the doc lags.

- **YAML `preceding_card: "上面有別人的名字。你已經看到了。"`** — pre-card.
  - **[是否實作?]** No — `EndingView::DrawEndingCard` (`EndingView.cpp:67-93`) renders only `title` + 1 hardcoded `caption(state)`; no preceding-card slot.
  - **[邏輯衝突?]** No (presentation-only; matches "narrative truth lives in jsonl/state, not pixels when assets absent" per CLAUDE.md §1).

- **YAML `unlocks: sfx_torn_paper (main menu)`** — audio unlock + main menu hook.
  - **[是否實作?]** No — no audio system in `src/`/`include/` (grep `sound|sfx|PlaySound` returns nothing); `TitleScreen.h:8-11` has only `StartGame|Quit` (no ending gallery / 結局圖鑑).
  - **[邏輯衝突?]** No (Raylib audio not wired; presentational future feature).

- **YAML `replay_allowed: true`** — restart from main menu allowed.
  - **[是否實作?]** Yes — pause-menu Restart rebuilds {World,View,GameController} (`CHANGELOG` Cycle-6 UI; `tests/test_restart_safety.cpp`).
  - **[邏輯衝突?]** No.

- **§一 開場字卡 + monologue prose (lines 14-31)** — opening titlecards + ambient description, system prompt 「Karma 已低於零」.
  - **[是否實作?]** No — these are blockquote prose under a `## 一、…` heading; loader's NPC scan requires `## NPC` prefix (`DialogLoader.cpp:42`), so the whole section is skipped and nothing renders past the 1-line caption.
  - **[邏輯衝突?]** No (presentation-only narrative; the `Karma < 0` system prompt aligns with the actual gate at `EndingGate.cpp:50`).

- **§二 五 NPC 的冰封反應: 西裝學長 (lines 41-53)** — cold-shoulder reaction beat.
  - **[是否實作?]** No — `### 西裝學長` is not a substate header (`DialogLoader.cpp:77` requires `### (` with `a..d`); section unrendered.
  - **[邏輯衝突?]** No (post-ending narrative; intentionally non-interactive).

- **§二 學霸 (lines 57-69)** — cold-shoulder reaction beat.
  - **[是否實作?]** No — same unrendered structure as above.
  - **[邏輯衝突?]** No.

- **§二 助教 (lines 73-86)** — returns the Ch1 申請書 影本; "把這份情還清了".
  - **[是否實作?]** No — unrendered prose; no Ch1 申請書 影本 item in `World.cpp` or `ChapterQuestItems.h`.
  - **[邏輯衝突?]** No (post-ending narrative continuity; not an unmet quest promise).

- **§二 福利社阿姨 (lines 90-103)** — terse "拿好哦" reaction.
  - **[是否實作?]** No — unrendered.
  - **[邏輯衝突?]** No.

- **§二 苦主 (lines 107-121)** — final 「我聽說了」 beat.
  - **[是否實作?]** No — unrendered.
  - **[邏輯衝突?]** No.

- **§三 主角內心獨白 (lines 123-147)** — 7-card monologue sequence.
  - **[是否實作?]** No — unrendered (rendered only by the single caption at `EndingView.cpp:35`).
  - **[邏輯衝突?]** No.

- **§四 結局字卡 Epilogue (lines 149-184)** — 7-card epilogue (5 NPC fates + protagonist envoi).
  - **[是否實作?]** No — unrendered; `EndingView` lacks a multi-card sequence.
  - **[邏輯衝突?]** No (matches BUGLEDGER V3 perception note: scripts `quit` ~28f after gate, full epilogue would require post-結局 hold).

- **§五 製作組說明 block (lines 186-202)** — replay note + sfx_torn_paper meta-spec.
  - **[是否實作?]** Partial — replay implemented (see `replay_allowed` row); sfx unimplemented; main-menu ending gallery unimplemented (`TitleScreen.h`).
  - **[邏輯衝突?]** No (future-feature meta-spec, not a contract).

## Summary

- Total elements: 16
- Yes: 3 · Partial: 2 · No: 11 · N/A: 0
- Conflicts: 2 (1 real doc-vs-code drift `hasItem("CursedUmbrella")` vocabulary; 1 real outdated third-trigger spec — YAML's "named umbrella from rack during Ch4" replaced by Cycle-6 L1 `coldFinale` and not back-ported into this file)
- Intentional (non-conflict): `karma<0` defensive lower-bound (F3a doc-only resolution)
- Dominant pattern: the file is a presentation/narrative spec; only the YAML state + caption + replay path are wired. Everything below the YAML (5 NPC reaction beats, monologue, epilogue, sfx hook) is unrendered — consistent with CLAUDE.md §1 "narrative truth is state.jsonl, not pixels" but means the file's body is *aspirational content*, not engine contract.
