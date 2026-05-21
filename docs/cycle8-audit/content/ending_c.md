# Audit — docs/content/ending_c.md

**Overview (≤3):**
1. `ending_c.md` is loaded into the runtime dialog table via `src/DialogSource.cpp:45` (`SemesterState::Ending_C` -> `"ending_c.md"`), but its contents are **字卡 narration only** — the in-game `DrawEndingCard` (`src/EndingView.cpp:36`) hard-codes a single caption (`「這樣以後再也不會有人拿錯你的傘了。」`) and `IsEndingState` is terminal: §2/§3/§4/§5 字卡 lines and the §3 NPC reactions are NOT presented by any current code path.
2. The §1 metadata `trigger_condition` is **partly true**: the Ch4 集英樓 Vendor purchase setting `Flag_BoughtUglyUmbrella` is real (`src/ChapterVendors.cpp:111-114`, `src/Vendor.cpp:91`, `src/EndingGate.cpp:69`); however `karma_range: any` / `prerequisite_flags: none` is now **misleading** post-Cycle-8 — `EndingGate.cpp:21` requires `Chapter4_Finals`, so Ending C is no longer "any playthrough" reachable from Ch1.
3. `game_coins == 0` (§1) and `unlock_reward: green_umbrella_icon_theme` / `replayable: true` (§1+§6) have **no implementation** — `Vendor::TryBuy` only checks `DeductMoney(item.price)` (price=100), there is no cosmetic icon-theme system, and there is no save/replay system in code.

## Per-element annotations

- **§1 `ending_id: ending_c` / `title: 破財消災`** — Metadata header.
  - **[是否實作?]** Yes — `SemesterState::Ending_C` (`include/SemesterState.h:15`); name 結局 C in `src/SemesterStateMachine.cpp:28`; 破財消災 strings in `include/GameHelp.h:34-36`, `src/EndingView.cpp:26`.
  - **[邏輯衝突?]** No.

- **§1 `trigger_condition: player enters 集英樓便利商店 → purchases 超醜螢光綠色雨傘`** — Ending C trigger.
  - **[是否實作?]** Yes — `src/ChapterVendors.cpp:111-114` 集英樓便利商店 stall sells `UglyUmbrella` (price 100, `setsFlag = "Flag_BoughtUglyUmbrella"`); `src/Vendor.cpp:91` writes the flag; `src/EndingGate.cpp:69` routes to `Ending_C`.
  - **[邏輯衝突?]** No (this is the canonical post-Cycle-8 trigger per CHANGELOG `b33db2b`).

- **§1 `game_coins == 0  # spends all coins on purchase`** — Player ends Ending C with 0 coins.
  - **[是否實作?]** No — `Vendor::TryBuy` (`src/Vendor.cpp:50-92`) requires only `DeductMoney(item.price)` (100); a player with >100 coins ends Ending C with leftover money. No "spend all" rule exists.
  - **[邏輯衝突?]** Yes — narrative claim vs implementation. REAL (unimplemented spec wording, not an intentional Cycle-N redesign — the `== 0` assertion appears nowhere in CHANGELOG/BUGLEDGER).

- **§1 `state_machine_entry: nccu::SemesterState::Ending_C`** — Target state.
  - **[是否實作?]** Yes — `include/SemesterState.h:15`; `src/SemesterStateMachine.cpp:62-66` handles transition.
  - **[邏輯衝突?]** No.

- **§1 `karma_range: any  # bypasses karma gating`** — No karma constraint.
  - **[是否實作?]** Partial — `EndingGate.cpp:69-71` does not check karma for the `Flag_BoughtUglyUmbrella` clause; however precedence A→B→C (`EndingGate.cpp:27,49`) means high-karma + ConsoledTA + TrueUmbrella overrides to A, and `Flag_TookCursedUmbrella || karma<0` overrides to B. So C is "any karma not already routed elsewhere."
  - **[邏輯衝突?]** No (defensible reading; CHANGELOG Cycle-3 F3a documents karma clamp behaviour).

- **§1 `prerequisite_flags: none  # available in any playthrough`** — Available any chapter.
  - **[是否實作?]** No — Cycle-8 (`src/EndingGate.cpp:21`) gates ALL ending checks behind `Chapter4_Finals`; the Ch1 阿姨 (c) buy is now an inert seed (CHANGELOG `b33db2b`, BUGLEDGER N2). Only the Ch4 集英樓 Vendor triggers it.
  - **[邏輯衝突?]** Yes — content claims "any playthrough"; reality is "Ch4 only." INTENTIONAL Cycle-8 redesign (BUGLEDGER N2 / CHANGELOG F1 `b33db2b`); content `ending_c.md` was not updated to match.

- **§1 `unlock_reward: green_umbrella_icon_theme  # main menu cosmetic`** — Post-clear cosmetic.
  - **[是否實作?]** No — `grep` for `icon_theme` / `green_umbrella` / cosmetic-unlock in `src/`/`include/` returns nothing. `TitleScreen.cpp` has no per-ending themed cursor.
  - **[邏輯衝突?]** Yes — wholly unimplemented. REAL (no CHANGELOG/BUGLEDGER reference; appears to be design-doc aspirational).

- **§1 `replayable: true`** — Re-entry without lock.
  - **[是否實作?]** Partial — pause-menu Restart (`include/World.h` `MenuOpen`/Cycle-6 UI shell) rebuilds {World,View,GameController} fresh (`tests/test_restart_safety.cpp`); state is reset, not preserved. No save/load system, so "doesn't lock save" is trivially true.
  - **[邏輯衝突?]** No.

- **§2 開場字卡 (5 字卡 lines: 「這樣以後再也不會有人拿錯你的傘了。」 + 4 more)** — Opening narration.
  - **[是否實作?]** Partial — only the FIRST 字卡 is rendered (`src/EndingView.cpp:36-37` hard-codes `「這樣以後再也不會有人拿錯你的傘了。」` as the single caption); the remaining 4 字卡 (line 27/29/31/33) are loaded into `nccu::dialog::Entries` but never presented (IsEndingState early-returns the world; no dialog runs in ending state).
  - **[邏輯衝突?]** Yes — 4 of 5 字卡 are dead content. REAL (no CHANGELOG entry repurposing them; consistent with V3 caption design from Cycle-3 R2 but unacknowledged that ending_c.md §2-§5 字卡 are not displayed).

- **§3 NPC 反應: 西裝學長 (5 lines)** — Suit senior reaction.
  - **[是否實作?]** No — Ending C is terminal (`EndingView.cpp:14-18` IsEndingState; `View.cpp:64` draws only `DrawEndingCard`); the player never controls anything post-`Transition(Ending_C)` so suit_senior dialog is never opened in ending state. NPC roster `ChapterSpawns.h:95 kEndingC` is `TODO(S5e): chapter roster`.
  - **[邏輯衝突?]** Yes — entire NPC-reaction section is dead. REAL (no CHANGELOG repurposes §3 as inert).

- **§3 NPC 反應: 學霸 (4 lines)** — Bookworm reaction.
  - **[是否實作?]** No — same root cause as 西裝學長 above; no Ending_C exploration phase.
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§3 NPC 反應: 助教 (5 lines)** — TA reaction.
  - **[是否實作?]** No — same root cause.
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§3 NPC 反應: 福利社阿姨 (5 lines)** — Auntie reaction.
  - **[是否實作?]** No — same root cause. (Note: Ch2 `DialogOpener.cpp:135` does open auntie (b) on `Flag_BoughtUglyUmbrella` BEFORE ending C is triggered, but that is `chapter2.md:255`, not `ending_c.md`.)
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§3 NPC 反應: 苦主 (5 lines)** — Victim reaction.
  - **[是否實作?]** No — same root cause.
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§4 主角內心獨白 (8 字卡)** — Inner monologue.
  - **[是否實作?]** No — same DrawEndingCard hardcode; not displayed.
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§5 結局 Epilogue (9 字卡)** — Closing epilogue.
  - **[是否實作?]** No — same; not displayed.
  - **[邏輯衝突?]** Yes — dead. REAL.

- **§6 製作組說明: 「本結局可重複進入，不鎖存檔」** — Replay claim.
  - **[是否實作?]** Partial — see §1 `replayable` above. There is no save-file at all, so "不鎖存檔" is trivially true; "本結局可重複進入" is achieved via the pause-menu Restart (Cycle-6 UI).
  - **[邏輯衝突?]** No.

- **§6 製作組說明: 「主選單解鎖螢光綠傘 icon 主題」** — Cosmetic unlock.
  - **[是否實作?]** No — see §1 unlock_reward; no cosmetic unlock subsystem.
  - **[邏輯衝突?]** Yes — unimplemented. REAL.

- **§6 製作組說明: 「三種結局各有側重」 (A/B/C tags)** — Designer note about three endings.
  - **[是否實作?]** Yes — `EndingView.cpp:30-40` and `EndingGate.cpp:19-75` implement all three.
  - **[邏輯衝突?]** No.

## Summary

- Elements documented: **18**
- Yes (fully implemented): **4** (ending_id/title, trigger_condition, state_machine_entry, three-endings architecture)
- Partial: **4** (karma_range, replayable, §2 opening — only 1/5, §6 replay clause)
- No (unimplemented): **10** (game_coins==0, prerequisite_flags=none, icon_theme reward, §2 4 字卡, all 5 §3 NPCs, §4 monologue, §5 epilogue, §6 cosmetic unlock; note: §3 counts as 5 separate NPC elements but folded for clarity)
- N/A: **0**
- Conflicts (REAL drift, no CHANGELOG redesign): **11** (game_coins==0, icon_theme, §2 4 dead 字卡, §3 5 NPCs treated as one bucket = 5 lines, §4 monologue, §5 epilogue, §6 cosmetic) — counted distinct annotation rows above.
- Conflicts (INTENTIONAL Cycle-N redesign with content not updated): **1** (`prerequisite_flags: none` — Cycle-8 N2/F1 consolidated to Ch4-only; content stale).

Total conflicts (M): **12** distinct annotation rows flagged Yes-cite.
Total elements (N): **18**.

DONE: /tmp/audit-cycle8/docs__content__ending_c.md (elements=18, conflicts=12)
