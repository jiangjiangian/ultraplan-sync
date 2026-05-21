# Audit — docs/content/chapter3.md

**Overview (≤3):**
1. Ch3 main spine (8 NPCs incl. 物物交換鏈 香腸→大聲公→學姊, +3/+3/+5; TrueUmbrella in 道具箱 = clear) is solidly implemented in `src/Chapter3Quest.cpp`, `include/Chapter3Quest.h`, `include/ChapterSpawns.h:52-69`, `src/DialogOpener.cpp:147-201`, `src/World.cpp:157-163`, `src/ChapterGate.cpp:23-26`, `include/EventWiring.h:56-65`.
2. Several documented chapter-opening mechanics are unimplemented: rainMeter doesn't seed to 50 on Ch3 entry (`Player::ApplyRain` starts at 0, no per-chapter init); umbrella isn't auto-cleared at Ch3 entry (only Ch4 does — `GameController.cpp:154-159` has Ch4-only branch). Both conflict with chapter3.md L6.
3. The 泥濘 -10% movement debuff and attribute "視野遮蔽" do not exist in code (no `Player` slow modifier, no terrain-based speed multiplier in `src/Player.cpp` / `src/GameController.cpp` / `src/TerrainMask.cpp`).

## Per-element annotations

- **Metadata: `SemesterState: Chapter3_SportsDay`** — chapter id. **[是否實作?]** Yes — `include/SemesterState.h:11`, `include/Chapter3SportsDay.h:9`, name "第三章 運動會". **[邏輯衝突?]** No.
- **Metadata: 起始 karma 沿用 Ch2 (×0.6)** — accumulative not reset. **[是否實作?]** Yes — `Player::karma_` persists across chapters (no per-chapter reset). **[邏輯衝突?]** No (×0.6 model never coded as such, but karma persistence is real).
- **Metadata: `rainMeter = 50` 起始** — half-day rain seed. **[是否實作?]** No — `Player.cpp:34` ctor `rainMeter_(0.0f)`; no Ch3-entry hook seeds 50. **[邏輯衝突?]** Yes (REAL — documented in chapter3 only, not in CHANGELOG as intentional removal).
- **Metadata: `hasUmbrella = false` 起始 (傘在大隊接力被偷)** — umbrella loss on Ch3 entry. **[是否實作?]** No — `GameController.cpp:154-159` only resets umbrella on `Chapter4_Finals` entry; Ch3 entry has no clear. **[邏輯衝突?]** Yes (REAL — Ch3 player keeps Ch1 TrueUmbrella, breaking the "find it again" narrative).
- **Metadata: 泥濘判定 -10% 移動速度** — slow on grass. **[是否實作?]** No — no speed modifier in `Player` / `GameController` / `TerrainMask`. **[邏輯衝突?]** Yes (REAL — feature unimplemented).
- **Metadata: 視野遮蔽 / 攤位密集** — camera/occlusion. **[是否實作?]** No — no occlusion code; vendors share Ch1 layout. **[邏輯衝突?]** Yes (REAL).
- **Metadata: 清關條件 持任意傘從體育館後台離開** — clear via TrueUmbrella claim. **[是否實作?]** Partial — clear is `Flag_Ch3Cleared` set by `EventWiring.h:56-65` upon `TrueUmbrella` claim at `World.cpp:160` {1500,1430}; "體育館後台" is narrative flavour, no spatial gate. **[邏輯衝突?]** No (loose match; intent met).
- **Metadata: 死亡傳送 `rainMeter ≥ 100` → 正門** — lethal rain. **[是否實作?]** Yes — `Player.cpp:124-126,146-148` lethal arms `RespawnAtGate()` at `Player.cpp:152`. **[邏輯衝突?]** No.
- **場景旁白 開場 + 系統訊息 (rain≥60/90/100, 進入草皮, 找到傘 etc.)** — flavour lines. **[是否實作?]** No — none of these are emitted by any code; not in `DialogSource.cpp` parse scope (旁白 sections are not NPC sections). **[邏輯衝突?]** No (intentional — narrative-only, not authored as triggered events).
- **物物交換鏈提示 旁白** — text descriptions of A/B/C trade. **[是否實作?]** Partial — `Chapter3Quest.cpp:21-49` emits its own `ShowMessage` strings (not the markdown 旁白 text verbatim). **[邏輯衝突?]** No (intentional Cycle-7/8 code-driven 香腸→大聲公→學姊 chain).
- **章節清關 旁白 + 字卡** — completion screen. **[是否實作?]** Partial — chapter advance via `ChapterGate.cpp:23-26` (Flag_Ch3Cleared → Interlude → Ch4) and `EventWiring.h:56-65`; no "字卡" rendered. **[邏輯衝突?]** No (字卡 only at endings per `EndingView.cpp`).
- **NPC 西裝學長 (a)/(b)/(c) + Flag_HelpedSenior / Flag_ScoldedSenior routing** — opener split. **[是否實作?]** Partial — `DialogOpener.cpp:178-186` routes HelpedSenior→(b) else (a); ScoldedSenior「不觸發對話」is a **KNOWN OMISSION** documented inline (L181-183). **[邏輯衝突?]** No (documented).
- **NPC 學霸 (a)/(b) Flag_BookwormRecovered split** — Ch2 ripple. **[是否實作?]** Yes — `DialogOpener.cpp:161-166` routes 0/1 by `kFlagBookwormRecovered`. **[邏輯衝突?]** No.
- **NPC 助教 (a)/(b)/(c) + Flag_HelpedTA_Ch1 routing** — Ch1 ripple. **[是否實作?]** Partial — `DialogOpener.cpp:167-171` routes (c) on `Flag_HelpedTA_Ch1`, else (a); (b) 「主線推進」not selectable via flag (parser flattens). **[邏輯衝突?]** No (routing OK; (b) is line-only recap).
- **NPC 福利社阿姨 (a)/(b)/(c)** — 應援道具情報. **[是否實作?]** Partial — present in `kChapter3` roster (`ChapterSpawns.h:61`); no Ch3-specific opener routing (default subState 0). **[邏輯衝突?]** No (passive ambient).
- **NPC 苦主 (a)/(b) Flag_PromisedVictim split + karma -5 隱性** — ripple disappointment. **[是否實作?]** Partial — `DialogOpener.cpp:172-177` routes (a) if `Flag_PromisedVictim` else (b); **karma -5 隱性 NOT applied** (no code-side karma deduction; the `// karma -5` is bullet-doc only — `Chapter3Quest.h:46-47` confirms "every Ch3 karma is path-b"). **[邏輯衝突?]** Yes (REAL — documented -5 penalty unimplemented; Ch3Quest.cpp has only the trade chain and ProfTrap ripple).
- **NPC A 系烤香腸攤主 (a)/(b) +3** — chain link 1. **[是否實作?]** Yes — `Chapter3Quest.cpp:16-25` sets `Flag_HasSausage` +3; `DialogOpener.cpp:187-192`. **[邏輯衝突?]** No.
- **NPC B 系大聲公持有者 (a)/(b) +3** — chain link 2. **[是否實作?]** Yes — `Chapter3Quest.cpp:27-37` clears 香腸, sets `Flag_HasLoudspeaker` +3. **[邏輯衝突?]** No.
- **NPC C 系學姊 (a)/(b) +5 情報** — chain link 3. **[是否實作?]** Yes — `Chapter3Quest.cpp:39-50` sets `Flag_KnowsUmbrellaLoc` +5. **[邏輯衝突?]** No.
- **分支一: TrueUmbrella → Chapter4_Finals** — main clear. **[是否實作?]** Yes — `World.cpp:157-163` spawns Umbrella at {1500,1430}; `EventWiring.h:63-65` advances to Interlude. **[邏輯衝突?]** No.
- **分支二: 持 ProfessorTrapUmbrella → -10 ripple** — Ch1 ripple. **[是否實作?]** Yes — `Chapter3Quest.cpp:53-65` `TryApplyCh3Ripple` -10 + `Flag_Ch3Rippled_ProfTrap` once-key. **[邏輯衝突?]** No (independent from Ch2 ripple per design).
- **分支三: 持 CursedUmbrella -5 環境懲罰 + NPC 各少一句熱情台詞** — cursed atmospherics. **[是否實作?]** No — no Ch3-specific cursed penalty; `DialogOpener.cpp` 物物交換鏈 routing doesn't gate on `Flag_TookCursedUmbrella`. **[邏輯衝突?]** Yes (REAL — feature unimplemented; cursed only affects ending via `EndingGate.cpp:49`).
- **分支四: rainMeter爆表 → 正門 + 部分情報永久錯過** — fail-state. **[是否實作?]** Partial — `RespawnAtGate` works (`Player.cpp:152-159`); "部分限時情報永久錯過" not modeled (no time-of-day tracker). **[邏輯衝突?]** No (lethal armed; "permanent missing" not gated).
- **分支五: Flag_BoughtUglyUmbrella 路徑計數 +1** — Ending C counter. **[是否實作?]** No — Ending C trigger is single Ch4 Vendor flag-set (per CHANGELOG N2 / `EndingGate.cpp:66`); no counter exists. **[邏輯衝突?]** No (INTENTIONAL Cycle-8 N2 redesign — explicitly consolidated; chapter3.md L350-356 prose is stale narrative flavour, no flag-set annotation).

## Summary

- Elements audited: **25**
- Yes (fully): **10**
- Partial: **8**
- No (unimplemented): **6**
- N/A: **0**
- Conflicts (REAL drift): **6** — rainMeter=50 init, hasUmbrella=false at Ch3 entry, 泥濘 -10% speed, 視野遮蔽, 苦主 -5 隱性 karma, 持 CursedUmbrella -5 + 台詞減少
- Conflicts (INTENTIONAL redesign): **0** (分支五 stale-text not flagged as conflict — explicit Cycle-8 N2 consolidation;旁白 omissions are intentional narrative-only).
