# Audit — docs/SCRIPT_HANDOFF.md

**Overview (≤3):**
1. SCRIPT_HANDOFF is the locked content/contract surface delivered to the implementation team. Cycle 8 just repointed §二 "Ending C 觸發點" to explicitly remove `Flag_KnowsUglyUmbrella` (now states the only trigger is Ch4 集英樓 Vendor → `Flag_BoughtUglyUmbrella` / `src/EndingGate.cpp:66`). After that edit the doc is largely conformant with code; remaining drift is small.
2. Almost all class contracts (§三), karma/money locks (§二), endings & state machine are implemented — `Player`, `EndingGate`, `ConsumableItem`, `Vendor`, factory, `SemesterStateMachine`, `DialogLoader` all match the prose. Idempotency guards landed in Cycle 6 (L2).
3. Genuine residual drift: (a) money soft-cap is 300 not 999 (intentional Cycle-1 retune, §二 doc lists 100 start but no cap; §五 still suggests "999 防超量" as unsettled); (b) `Flag_BoughtCoffeeForAuntie_Ch1` is still listed as setter-pending in §三 even though Cycle-1 B3 wired the Ch1 (d) coffee choice; (c) `MapManager::ApplyStateOverlay` Observer is not implemented as a class (state-overlay is handled implicitly via `ChapterSpawns`/`ChapterGate`).

## Per-element annotations

- **Files in §一 (9 docs, 2745 lines)** — Lockdown list of authored content.
  - **[是否實作?]** Yes — all 9 files present under `docs/content/`, loaded by `src/DialogSource.cpp:38-45` + `src/DialogLoader.cpp`.
  - **[邏輯衝突?]** No.

- **Karma 起始 = 50** — Initial karma constant.
  - **[是否實作?]** Yes — `src/Player.cpp:34` `karma_(50)`.
  - **[邏輯衝突?]** No.

- **Karma 刻度 ±3/±5/±10 (−15/−30 大事)** — Tier scale.
  - **[是否實作?]** Yes — Cycle-3 F2 lowered CursedUmbrella `−50→−30` to match (`include/CursedUmbrella.h`); content uses −3/+3/±5/−10/−15 throughout (`docs/content/chapter1.md`).
  - **[邏輯衝突?]** No (post-F2).

- **Ending A 門檻 `karma > 80`** — Threshold.
  - **[是否實作?]** Yes — `src/EndingGate.cpp:27` `GetKarma() > 80`.
  - **[邏輯衝突?]** No.

- **Ending B 門檻 `karma < 0`** — Threshold.
  - **[是否實作?]** Yes (defensive lower bound) — `src/EndingGate.cpp:51`. F3a doc-clarified Cycle-3 that the practical driver is `Flag_TookCursedUmbrella` / cold finale.
  - **[邏輯衝突?]** No (INTENTIONAL Cycle-3 F3a doc clarification + Cycle-6 L1 cold-finale extension).

- **money 起始 100 / soft-cap 999** — Economy bounds.
  - **[是否實作?]** Partial — start 100 ✓ (`src/Player.cpp:34`); soft-cap is **300** not 999 (`include/Player.h:32 kMoneySoftCap = 300`).
  - **[邏輯衝突?]** Yes (doc-vs-code) — INTENTIONAL: §五 lists 999 as "尚未拍板"; Player.h:29 comment says "999 retuned" to 300 for the 3-cycle economy.

- **雨表 ≥100 → 正門, 推半天, 不扣 karma** — Death penalty.
  - **[是否實作?]** Yes — `src/Player.cpp:152 RespawnAtGate` (no karma touch); lethal wired Cycle-4 I8, drain rule + 3-way GC tick.
  - **[邏輯衝突?]** No.

- **對白 ≤ 28 全形字 / (a)(b)(c)(d) substates / `// karma ±N`** — Format contract.
  - **[是否實作?]** Yes — `DialogLoader.cpp:83` caps `'a'..'d'`; B4 enforced 28-cell wrap (`DialogLayout`); `dialog_lint.py` validates.
  - **[邏輯衝突?]** No.

- **Ending C 觸發點 = Ch4 集英樓 Vendor only; Ch1 阿姨 (c) 純敘事種子, no flag** — Cycle 8 repoint.
  - **[是否實作?]** Yes — `src/EndingGate.cpp:66-68` reads `Flag_BoughtUglyUmbrella`; Cycle-8 N2 (`b33db2b`) removed `Flag_KnowsUglyUmbrella` from `chapter1.md`, `Harness.cpp KnownFlags`, EndingGate header.
  - **[邏輯衝突?]** No (this is the Cycle-8 fix itself).

- **NPC dialog 注入 (SetDialogLines)** — Contract.
  - **[是否實作?]** Yes — runtime loading via `DialogLoader::LoadChapter` (per CLAUDE.md §2 #3); contract honoured.
  - **[邏輯衝突?]** No.

- **Karma 變化注入 (OnInteract hook)** — Contract.
  - **[是否實作?]** Yes — DialogChoice `karmaDelta` field + ApplyDialogChoice; quest hooks (Chapter*Quest).
  - **[邏輯衝突?]** No.

- **Flag whitelist — Ch1 setters:**
  - **`Flag_HelpedSenior` / `Flag_ScoldedSenior`** — Senior reactions.
    - **[是否實作?]** Yes — content + `DialogOpener.cpp:100-101`; Cycle-8 N1 (`496a771`) finally wired Ch1 (b) `Flag_ScoldedSenior = true`.
    - **[邏輯衝突?]** No (post-N1).
  - **`Flag_HasProfessorTrap`** — `src/ProfessorTrapUmbrella.cpp:13`. **[是否實作?]** Yes. **[邏輯衝突?]** No.
  - **`Flag_TookCursedUmbrella`** — `src/CursedUmbrella.cpp:14`. **[是否實作?]** Yes. **[邏輯衝突?]** No.
  - **`Flag_HelpedTA_Ch1`** — Quest hook. **[是否實作?]** Yes (read in `Chapter4Quest.cpp:44`, `DialogOpener.cpp:77/108/169/230`). **[邏輯衝突?]** No.
  - **`Flag_PromisedVictim` / `Flag_SawVictim_Ch1`** — **[是否實作?]** Partial — `Flag_PromisedVictim` Yes (`TransparentUmbrella.cpp:15`, opener routing). `Flag_SawVictim_Ch1` **removed** (B3 dead-flag) but **still listed in SCRIPT_HANDOFF §三**. **[邏輯衝突?]** Yes (doc-vs-code, stale-doc-only) — INTENTIONAL Cycle-1 B3 removal not yet reflected in §三 whitelist text.
  - **`Flag_BoughtUglyUmbrella`** — `src/EndingGate.cpp:66`. **[是否實作?]** Yes. **[邏輯衝突?]** No.
  - **`Flag_BoughtCoffeeForAuntie_Ch1`** — Doc says "建議在 Ch1 補一個觸發點". **[是否實作?]** Yes — Cycle-1 B3 wired Ch1 阿姨 (d) 請咖啡 (sets flag, +5 karma; `Chapter4Quest.cpp:30` reads it). **[邏輯衝突?]** Yes (stale-doc-only) — §三 still says "建議補一個觸發點" although it's been wired since Cycle-1.

- **Flag whitelist — Ch2 setter `Flag_BookwormRecovered`** — Wake bookworm.
  - **[是否實作?]** Yes — `Chapter4Quest.cpp:22`, gated by Ch2 quest.
  - **[邏輯衝突?]** No.

- **Ch3 reuse of Ch1/Ch2 flags** — Statement.
  - **[是否實作?]** Yes — `Chapter3Quest.cpp` uses `Flag_HasProfessorTrap`; barter chain adds `kFlagHasSausage` / loudhailer flags.
  - **[邏輯衝突?]** No.

- **ConsumableItem / HotPack / WaterproofSpray / EnergyDrink / CashPickup / Vendor classes** — Class contracts.
  - **[是否實作?]** Yes — all headers present (`include/ConsumableItem.h`, `HotPack.h`, `WaterproofSpray.h`, `EnergyDrink.h`, `CashPickup.h`, `Vendor.h`).
  - **[邏輯衝突?]** No.

- **GameObjectFactory CreateNPC/CreateUmbrella/CreateConsumable/CreateVendor/CreateCashPickup** — Factory API.
  - **[是否實作?]** Partial — `GameObjectFactory::Create(ObjectType, Vec2)` covers umbrellas/consumables/cash/Vendor stub but Vendor stalls are built directly in `main.cpp` with `VendorConfig` (factory comment acknowledges); no `CreateNPC` method.
  - **[邏輯衝突?]** Yes (doc-vs-code) — INTENTIONAL: factory comment (`include/GameObjectFactory.h:16-18`) explains stalls bypass the factory by design.

- **SemesterStateMachine 5 states + Ending A/B/C** — State machine.
  - **[是否實作?]** Yes — `include/SemesterState.h:8-15` lists all 8 states; `include/SemesterStateMachine.h` implements transitions.
  - **[邏輯衝突?]** No.

- **`MapManager::ApplyStateOverlay(state)` Observer** — Map-overlay hook.
  - **[是否實作?]** No — no `MapManager` class anywhere in `src/include/`; state-driven roster is handled by `ChapterSpawns.h` + `ChapterGate.cpp` directly.
  - **[邏輯衝突?]** Yes (doc-vs-code) — INTENTIONAL: an architectural alternative (per CLAUDE.md §3 design-governance); doc says "interface draft".

- **新 NPC 清單 (圖書館管理員 / A 系香腸 / B 系大聲公 / C 系學姊 / 10 Vendors / 公告板)** — Roster.
  - **[是否實作?]** Yes — `librarian` (`ChapterSpawns.h:43`, `DialogSource.cpp:23`); `vendor_sausage_a` / loudhailer / 學姊 (`Chapter3Quest.cpp:16-47`); 10 Vendors (Cycle-7 R6/R7); 公告板 via `Flag_LeaveInterlude` (`ChapterGate.cpp:37`).
  - **[邏輯衝突?]** No.

- **§五 待覆核 (sprite reuse, 自販機, 體育館道具箱, Ch4 傘架 etc.)** — Open knobs.
  - **[是否實作?]** N/A — by definition unsettled. Most decisions taken in subsequent cycles (Cycle-7 R6 distinct sprites; Cycle-1 EnergyDrink/Vendor wiring).
  - **[邏輯衝突?]** No.

- **§六 implementation order (10 steps)** — Recommended sequence.
  - **[是否實作?]** Yes — all 10 steps complete (basic classes, factory, 4 umbrellas, 3 consumables, Vendor, SemesterStateMachine, dialog load, EventBus, deferred deletion).
  - **[邏輯衝突?]** No.

## Summary
- **Implemented:** 25
- **Partial:** 2 (money cap; GameObjectFactory CreateNPC)
- **Not-implemented:** 1 (MapManager::ApplyStateOverlay class)
- **Conflicts (doc-vs-code):** 4 — money cap (intentional retune), `Flag_SawVictim_Ch1` stale in §三 (B3 removal), `Flag_BoughtCoffeeForAuntie_Ch1` "建議補" stale (B3 wired it), MapManager Observer absent (intentional architectural alt)
- **Stale-doc-only (no behavioural drift):** 2 (the two Flag_* entries above)
