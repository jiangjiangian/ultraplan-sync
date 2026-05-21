# Audit — docs/ROADMAP.md

**Overview (≤3):**
1. ROADMAP is a Cycle-0 forecast: it locks C++17, prescribes a `tools/gen_dialog.py` codegen pipeline producing `include/DialogData.h`, and predicts a 40-line `main.cpp`. At HEAD `b33db2b` the project is C++20 (CMakeLists.txt: `CMAKE_CXX_STANDARD 20`), there is NO `tools/` directory nor `DialogData.h`, and `main.cpp` is 67 lines wrapping a Title→Select→Playing screen loop. Dialogue uses the runtime `DialogLoader` instead of build-time codegen.
2. The "缺口/能不能玩" gap list (1–5) and the §六 Tier-3 deferrals are mostly RESOLVED — see CHANGELOG Cycles 1–8: dialog box (B4), DialogSession (live),章節 gating (EndingGate L1 TOTAL gate), rain economy (I8→Cycle-4 drain rule; R5 sheltered-rate per-chapter pressure), Vendor::TryBuy (I5), Tab inventory, ending cards (V3), market 10-vendor cluster (R6/R7), 4 distinct umbrellas (R9), three endings reachable & 2× byte-identical (I7).
3. Most ROADMAP claims are **Stale-doc-only**: the doc is behind a delivered system. The one **Real conflict** is the C++17 mandate on line 67/396 vs CMakeLists C++20 (Cycle-1 raised the standard). ROADMAP also mis-states `main.cpp` size (40 → 67), Tier 3 ProfessorTrap chase (still simulated TODO — Stale-doc-only as it is also deferred), and 圖書館慢走 (still not implemented — matches Tier-3 deferred).

## Per-element annotations

- **Tier 2 scope = Phase 0+1+2 = complete plot (line 8)** — completion bar.
  - [是否實作?] Yes — `.claude/CHANGELOG.md` Cycle-1..8; 3 endings 2× byte-identical (CHANGELOG 2026-05-19 I7).
  - [邏輯衝突?] No — Stale-doc-only (forecast → done).

- **Dialogue = build-time codegen (line 8, §二)** — chose codegen approach.
  - [是否實作?] No — no `tools/gen_dialog.py`, no `include/DialogData.h`. Runtime `DialogLoader.cpp` is used instead.
  - [邏輯衝突?] Yes — **Stale-doc-only** (architecture pivoted to runtime load; B4 layered `DialogLayout` on top).

- **"沿用現有 worldmap.png + camera-follow" (line 12)** — no minimap.
  - [是否實作?] Yes — `src/View.cpp` Camera2D unchanged.
  - [邏輯衝突?] No.

- **§一 已完成 class tree (lines 23–25)** — GameObject/Character/Item hierarchy.
  - [是否實作?] Yes — `include/GameObject.h`, `include/Vendor.h`, `include/GameObjectFactory.h`.
  - [邏輯衝突?] No.

- **§一 已完成: MVC purified, main = 40 行 (lines 26–27)** — composition root size.
  - [是否實作?] Partial — MVC purified Yes; `src/main.cpp` is **67 lines** (Title→Select→Playing screen-flow loop added per Cycle 6 UI shell).
  - [邏輯衝突?] Yes — **Stale-doc-only** (line-count claim outdated by Cycle 6 `6d9ca97`).

- **§一 已完成: Player fields rainMeter/karma=50/money=100/flags (lines 28–29)** — Player state.
  - [是否實作?] Yes — `include/Player.h`.
  - [邏輯衝突?] No.

- **§一 已完成: SemesterState 8 states (lines 30–32)** — state machine.
  - [是否實作?] Yes — `include/SemesterStateMachine.h`.
  - [邏輯衝突?] No.

- **§一 已完成: tests 19 檔 / 93 case / 290 斷言 (line 35)** — test counts.
  - [是否實作?] Partial — Tests exist & expanded to **289/289 cases / 4254 assertions** (Cycle-8 gate); the snapshot is far behind.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **缺口 1: docs/content 2745 行劇本零接入 (lines 39–40)** — content not loaded.
  - [是否實作?] No (claim) — content IS loaded since pre-Cycle-1 via `DialogLoader.cpp`/`DialogSource.cpp`; B3/B5/N1/N2 all edited live content.
  - [邏輯衝突?] Yes — **Stale-doc-only** (claim no longer true).

- **缺口 2: 無對話框 UI 與流程 (lines 41–42)** — DialogBox/branch.
  - [是否實作?] Yes — `include/DialogView.h`, `src/DialogView.cpp`, `include/DialogState.h`, `include/DialogLayout.h` (B4 wrap+paginate); DialogChoice + flag/karma in `src/DialogOpener.cpp`.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **缺口 3: rainMeter 只累積不懲罰 (lines 43–44)** — passive rain.
  - [是否實作?] Yes — Cycle-4 I8 lethal+drain; Cycle-7 R5 ApplyRainSheltered per-chapter pressure; `src/GameController.cpp` 3-way tick.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **缺口 4: 章節推進是假的 enterTrigger_ (lines 45–46)** — soft gating.
  - [是否實作?] Yes — Quest/flag-gated semester progression in `src/EventWiring.cpp` (UmbrellaClaimed advances), `EndingGate.cpp` TOTAL gate (Cycle-6 L1).
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **缺口 5: 三結局無演出/無 Tab 物品欄/ProfTrap TODO/圖書館慢走/無存檔 (lines 47–48)** — bundle of 5.
  - [是否實作?] Partial — Endings演出 Yes (V3); Tab 物品欄 Yes (`InventoryOpen`); ProfTrap chase **No** (still simulated); 圖書館強制慢走 **No**; 存檔 **No**.
  - [邏輯衝突?] No — Stale-doc-only for the 2 done; the 3 undone match §六 Tier-3 deferrals.

- **§二 codegen pipeline: tools/gen_dialog.py + include/DialogData.h (lines 56–60)** — architecture.
  - [是否實作?] No — neither file exists.
  - [邏輯衝突?] Yes — **Stale-doc-only** (project chose runtime load).

- **§二 C++17, no std::span, pointer+count (line 67)** — language constraint.
  - [是否實作?] No — CMakeLists `CMAKE_CXX_STANDARD 20` since Cycle-1.
  - [邏輯衝突?] Yes — **Real conflict** (the doc still mandates C++17; current code is C++20). CLAUDE.md §5 also explicitly permits C++20+.

- **§二 DialogBox uses IRenderer::DrawRect placeholder, 28 全形字 (lines 82–84)** — render plan.
  - [是否實作?] Yes — `DialogView.cpp` + `DialogLayout::CellWidth/Paginate` enforcing 28 cells (B4).
  - [邏輯衝突?] No — Stale-doc-only (delivered).

- **§二 DialogSession + E推進 + 選項 (lines 85–88)** — execution plan.
  - [是否實作?] Yes — `src/DialogOpener.cpp` ApplyDialogChoice + GameController E-key.
  - [邏輯衝突?] No.

- **§二 旗標清單 Ch1/2 (lines 91–96)** — flag whitelist.
  - [是否實作?] Partial — all listed flags exist in `SCRIPT_HANDOFF.md` whitelist and most are wired; **`Flag_BoughtUglyUmbrella` set by Ch4 集英樓 Vendor** not Ch1 (per N2 Cycle-8 clarification).
  - [邏輯衝突?] No — Stale-doc-only (flag now lives in Ch4, not Ch1).

- **§二 鎖定數值 (lines 99–103) — karma 50 / ±3±5±10 / -15-30 / >80 / <0 / money 100 / rain ≥100→正門 / Ending C 集英樓 便利商店** — locked constants.
  - [是否實作?] Yes — `EndingGate.cpp` thresholds; CursedUmbrella penalty was rebalanced 50→30 in Cycle-3 F2 (still within −15/−30); rain teleport LIVE since Cycle-4.
  - [邏輯衝突?] No.

- **Phase 0 交付: gen_dialog.py + DialogData.h + parser unit test (lines 110–115)** — milestone.
  - [是否實作?] No — pivoted to runtime DialogLoader.
  - [邏輯衝突?] Yes — **Stale-doc-only**.

- **Phase 1 交付: DialogBox/DialogSession/Ch1 chain/章節閘門/Ending 字卡模板 (lines 119–128)** — Ch1 spine.
  - [是否實作?] Yes — all delivered through Cycles 1–8 (I7 spine, V3 cards, L1 gate).
  - [邏輯衝突?] No.

- **Phase 2 交付: Interlude market 10 Vendor + Tab + Ch2 圖書館 + Ch3 物物交換 + Ch4 漣漪 + 3結局演出 (lines 134–149)** — full plot.
  - [是否實作?] Yes — Cycle-7 R6/R7 (10-vendor centred cluster), Ch2 學霸+管理員, Ch3 香腸→大聲公→學姊, Ch4 助教 finale (#8 verified), 3 endings via I7 spine.
  - [邏輯衝突?] No.

- **§四 美術 resources/assets/ui 落地即換 (lines 158–168)** — concurrent art plan.
  - [是否實作?] Partial — code paths exist; runtime art is largely curated-out per CLAUDE.md `resources/`-empty fresh clone.
  - [邏輯衝突?] No.

- **§五 共同驗收 gate (lines 173–179)** — gate criteria.
  - [是否實作?] Yes — Cycle-8 gate independently met: 0 project warnings, ctest 289/289, dialog_lint exit 0.
  - [邏輯衝突?] No.

- **§六 Tier 3 打磨 deferred (lines 185–188): rain 50/80 降速 + 暗角 / Shift 奔跑 / 圖書館慢走 + 安靜BGM / Ch3 帳篷 overlay / ProfTrap AI / 音效/BGM / 存檔** — deferred polish.
  - [是否實作?] Partial — rain 暗角 vignette done (V2); 50%/80% 降速 **No** (CHANGELOG Cycle-3 explicit); Shift 奔跑 **No**; 圖書館強制慢走 **No** (grep confirms); Ch3 帳篷 overlay **No**; ProfTrap chase AI **No**; 音效/BGM **No**; 存檔 **No**.
  - [邏輯衝突?] No — these remain deferred per the doc itself.

## Summary

- **Total elements audited:** 22
- **Yes:** 13 — **Partial:** 5 — **No:** 4 — **N/A:** 0
- **邏輯衝突 (Yes):** 10 — of which **Stale-doc-only:** 9; **Real conflict:** 1 (C++17 mandate, line 67/396, contradicts CMakeLists C++20)
- Per CLAUDE.md §2 the ROADMAP's "current status" prose is historical; the Phase-0 codegen pipeline architecture was abandoned for runtime DialogLoader, and Phase-1/2 milestones have shipped through Cycles 1–8.
