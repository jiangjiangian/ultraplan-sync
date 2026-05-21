# Audit — docs/content/chapter2.md

**Overview (≤3 sentences):** chapter2.md describes Ch2 (圖書館期中考) with 6 NPC sections, a 3-note collection quest, EnergyDrink rescue of 學霸, FragileUmbrella librarian loan, and chapter-special mechanics (forced slow-walk, white-noise BGM, +20% rain rate, front-gate respawn). Core spine (notes → wake bookworm → TrueUmbrella retrieve → clear) is fully implemented; cross-chapter ripples (HelpedSenior/ScoldedSenior/HasProfessorTrap/HelpedTA_Ch1/PromisedVictim/BoughtUglyUmbrella/TookCursedUmbrella) all routed by `DialogOpener.cpp:99-145`. Several supporting mechanics (slow-walk, white-noise BGM, +20% rain rate, ChapterGate→正門 on death) are documented but not coded; one real labelling/routing conflict in 學長 (a) heading; intentional Cycle-8 redesign repurposes `Flag_ScoldedSenior` to be settable from Ch1 (b).

## Per-element annotations

- **章節 metadata: `SemesterState = Chapter2_Midterms`** — chapter id.
  - [是否實作?] Yes — `include/SemesterState.h:10`, `include/Chapter2Midterms.h:9`, `src/SemesterStateMachine.cpp:48`.
  - [邏輯衝突?] No.

- **起始條件: rainMeter = 0 (Interlude後歸零), karma 沿用, hasUmbrella=false** — entry state.
  - [是否實作?] Partial — Interlude entry repositions player + `ClearConsumables()` (`src/GameController.cpp:136-146`) but no `resetRainMeter()` nor `SetHasUmbrella(false)` on Ch2 entry. Only `Chapter4_Finals` resets umbrella (`GameController.cpp:153-158`). `rainMeter` only resets on `RespawnAtGate` (`src/Player.cpp:157`).
  - [邏輯衝突?] Yes (real) — doc says rain & umbrella reset on Ch2 entry; engine carries them through. Player can keep Ch1 TrueUmbrella into Ch2.

- **清關條件: `inventory.hasItem("TrueUmbrella")` 且回到正門** — clear trigger.
  - [是否實作?] Partial — engine uses `Flag_Ch2Cleared` (`Chapter2Quest.cpp:50`, `ChapterGate.cpp:15-19`) lifted after `kFlagBookwormRecovered` + dialog closed; no inventory/location check. Bookworm 換回傘 path implicitly grants TrueUmbrella via game design but `LiftChapter2Clear` only checks recovered flag.
  - [邏輯衝突?] No (Cycle-redesign per CLAUDE.md §3; the flag is the actual gate).

- **Karma 刻度: 累積型(×0.6)沿用** — karma scale.
  - [是否實作?] No — karma is integer, AddKarma direct sums (`Player::AddKarma`); no ×0.6 multiplier.
  - [邏輯衝突?] No (numbers-baseline per CLAUDE.md §3; intentional simplification).

- **圖書館強制慢走 70%** — speed reduction inside library.
  - [是否實作?] No — `grep MovementSpeed/slowwalk/library` finds no speed scaling tied to `中正圖書館` (`include/Buildings.h:52`).
  - [邏輯衝突?] No (documented gameplay mechanic deferred).

- **白噪音 BGM 切換** — audio swap inside library.
  - [是否實作?] No — no BGM/audio system in src/include.
  - [邏輯衝突?] No (no audio subsystem; deferred).

- **雨量強度 +20%** — Ch2 rain rate.
  - [是否實作?] No — `Player::ApplyRain` hardcodes 5.0 u/s (`Player.cpp:123`); no per-chapter multiplier.
  - [邏輯衝突?] No (CHANGELOG R5 documents Ch2 peaks 52-81 already; multiplier is doc-aspirational).

- **正門結算區 (rainMeter=100 強制傳送, 無 karma 扣)** — death respawn.
  - [是否實作?] Yes — `Player::RespawnAtGate()` (`Player.cpp:152-159`) teleports to {500,1860}, resets rain, ShowMessage, no karma penalty.
  - [邏輯衝突?] No.

- **場景旁白：開場/系統訊息/章節清關 (8 旁白組)** — narrative text.
  - [是否實作?] Partial — text lives in markdown; DialogLoader reads `## NPC：` only; freestanding `###` blocks under `## 章節 metadata` / `## 場景旁白` are NOT parsed as NPC dialog. They display only if triggered through code (no such trigger exists).
  - [邏輯衝突?] No (documentation tier; not parser-bound).

- **NPC 西裝學長 (a) "Flag_ScoldedSenior=true 或 karma 中性"** — passing greeting.
  - [是否實作?] No — `DialogOpener.cpp:100-102` routes ScoldedSenior→(c), HelpedSenior→(b), default→(a). Doc's `(a)` heading inverts the engine routing.
  - [邏輯衝突?] **Yes (real mislabel)** — (a) is the NEUTRAL/no-flag path (`return 0`), not ScoldedSenior. Heading wording contradicts routing.

- **NPC 西裝學長 (b) HelpedSenior, karma +3** — friendly callback.
  - [是否實作?] Yes — routed `DialogOpener.cpp:100`; karma +3 landed by `TryApplyCh2Ripple` (`Chapter2Quest.cpp:64-65`).
  - [邏輯衝突?] No.

- **NPC 西裝學長 (c) ScoldedSenior, karma −3** — cold path.
  - [是否實作?] Yes — routed `DialogOpener.cpp:101`; karma −3 by `Chapter2Quest.cpp:66-67`. CHANGELOG Cycle-8 N1 made `Flag_ScoldedSenior` reachable from Ch1 (b).
  - [邏輯衝突?] No.

- **NPC 學霸 (a) 三樓初遇** — default ghost-mode.
  - [是否實作?] Yes — `DialogOpener.cpp:127-129` returns 0 when not recovered, not cursed.
  - [邏輯衝突?] No.

- **NPC 學霸 (b) TookCursedUmbrella 冷反應** — cursed variant.
  - [是否實作?] Yes — `DialogOpener.cpp:128` routes flag→1.
  - [邏輯衝突?] No.

- **NPC 學霸 (c) EnergyDrink 喚醒, 6行對白** — rescue path.
  - [是否實作?] Partial — `TryRescueBookworm` (`Chapter2Quest.cpp:22-32`) consumes EnergyDrink + sets recovery flag + ShowMessage; but the (c) dialog block itself NOT routed (BUGLEDGER §F.5 KNOWN OMISSION — DialogOpener.cpp:122-126 comment confirms (c)/(c-fail) not separable in parser).
  - [邏輯衝突?] No (CHANGELOG-acknowledged omission).

- **NPC 學霸 (c-fail) 無飲料 fallback** — locked-out hint.
  - [是否實作?] Partial — fallback ShowMessage in `Chapter2Quest.cpp:38-40` ("圖書館地下室自動販賣機 35 元"); dialog (c-fail) sub-block not routed (same parser limit).
  - [邏輯衝突?] No.

- **NPC 學霸 (d) 感謝, karma +5** — recap.
  - [是否實作?] Yes — karma +5 lands in `TryRescueBookworm` (`Chapter2Quest.cpp:29`); (d) recap routed `DialogOpener.cpp:127` (`kFlagBookwormRecovered`→3).
  - [邏輯衝突?] No.

- **NPC 助教 (a) 基本出場** — default.
  - [是否實作?] Yes — `DialogOpener.cpp:109` `return 0`.
  - [邏輯衝突?] No.

- **NPC 助教 (b) HelpedTA_Ch1, 主動提供位置** — info ripple.
  - [是否實作?] Yes — `DialogOpener.cpp:108` flag→1.
  - [邏輯衝突?] No.

- **NPC 助教 (c) HasProfessorTrap, karma −10** — Ch1 ripple.
  - [是否實作?] Yes — routed `DialogOpener.cpp:107`; −10 landed by `Chapter2Quest.cpp:79-81`.
  - [邏輯衝突?] No.

- **NPC 福利社阿姨 (a) 進店** — default.
  - [是否實作?] Yes — `DialogOpener.cpp:135-136` default 0.
  - [邏輯衝突?] No.

- **NPC 福利社阿姨 (b) BoughtUglyUmbrella 認出** — Ch4 ripple recall.
  - [是否實作?] Yes — `DialogOpener.cpp:135` flag→1.
  - [邏輯衝突?] No.

- **NPC 福利社阿姨 (c) rainMeter≥60 狼狽版** — high-rain variant.
  - [是否實作?] No — `DialogOpener.cpp:131-137` only routes on flags, not rainMeter. Doc itself flags this as not opener-routed ("非 opener 路由").
  - [邏輯衝突?] No (documented KNOWN OMISSION).

- **NPC 苦主 (a) 路過 (Ch1未承諾)** — default.
  - [是否實作?] Yes — `DialogOpener.cpp:144` default 0.
  - [邏輯衝突?] No.

- **NPC 苦主 (b) 二次互動** — generic.
  - [是否實作?] Partial — substate exists but only (c)/(d) flag branches are routed; default chain goes (a)→(c)/(d) by flag, never explicitly to (b).
  - [邏輯衝突?] No (acceptable opener pattern).

- **NPC 苦主 (c) PromisedVictim 承諾兌現** — Ch1 ripple.
  - [是否實作?] Yes — `DialogOpener.cpp:142` flag→2.
  - [邏輯衝突?] No.

- **NPC 苦主 (d) BoughtUglyUmbrella 醜傘辨識** — Ch1 ripple alt.
  - [是否實作?] Yes — `DialogOpener.cpp:143` flag→3 (precedence: PromisedVictim > Ugly).
  - [邏輯衝突?] No.

- **NPC 圖書館管理員 (a) 詢問線索** — info hub.
  - [是否實作?] Yes — spawned `ChapterSpawns.h:42-43` (`librarian`, isQuestGiver=true); `DialogOpener.cpp:116-117` default 0.
  - [邏輯衝突?] No.

- **NPC 圖書館管理員 (b) 收齊筆記後** — gate to 羅馬廣場.
  - [是否實作?] Yes — `Chapter2NotesComplete` → `DialogOpener.cpp:116` returns 1.
  - [邏輯衝突?] No.

- **散落筆記 3 頁 (各 +3 karma 合計)** — collectibles.
  - [是否實作?] Yes — `ChapterQuestItems.h:40-47` 3 placements, kNoteSet completion karma 3 (granted once via QuestFlagPickup, see comment 31-34).
  - [邏輯衝突?] No.

- **FragileUmbrella 管理員借出 (20% rate)** — loan umbrella.
  - [是否實作?] Partial — FragileUmbrella exists (`src/FragileUmbrella.cpp`), spawned ctor at {750,1280} (`World.cpp:30`), `SetHasUmbrella(true)`. **No librarian-give event:** the umbrella is a world pickup, not handed out after notes. 20% reduced rate also absent (FragileUmbrella behaves like any umbrella vs `ApplyRainSheltered` 1.5 u/s).
  - [邏輯衝突?] No (engine uses generic shelter rate; doc-aspirational specificity).

- **EnergyDrink 圖書館地下室自販機 35元 (anti-softlock)** — fallback vendor.
  - [是否實作?] Yes — `ChapterVendors.cpp:89-97` Chapter2Vendors at {660,1850}.
  - [邏輯衝突?] No.

- **章節結尾分支提示 (TrueUmbrella/ProfTrap/Cursed/UglyUmbrella/death)** — outcome flavour text.
  - [是否實作?] Partial — branches are documentation; ripple karma effects are coded (`TryApplyCh2Ripple`, `Chapter2Quest.cpp`), but no card/字卡 rendering for Ch2 (字卡 only for endings A/B/C per `EndingView`). "字卡：『幾週後…』" not implemented.
  - [邏輯衝突?] No (intentional — chapter transitions are seamless per CHANGELOG).

- **補設定 (1–7): 學霸位置/備用傘/c-fail 替代/Ending C路徑/管理員人設/助教(b)/學長地點** — author補注.
  - [是否實作?] N/A — meta annotations; substantive items (3 notes spawn / vendor fallback / opener routing / librarian quest-giver) are coded as cited above.
  - [邏輯衝突?] No.

## Summary

- Elements audited: **33**
- Yes: 17 · Partial: 8 · No: 7 · N/A: 1
- Real conflicts: **1** (學長 (a) heading mislabels routing — (a) is default/neutral, not ScoldedSenior)
- Intentional Cycle-N redesigns: 4 (清關 flag-driven not inventory-driven; karma not ×0.6; rain rate not Ch2-multiplied; ending 字卡 absent)
- Documented KNOWN OMISSIONs (CHANGELOG/source comments): 3 (學霸 (c)/(c-fail) parser flatten; 阿姨 (c) rainMeter route; FragileUmbrella librarian-give)
