#ifndef CHAPTER1_QUEST_H_
#define CHAPTER1_QUEST_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the return / grant

namespace nccu {

class DialogState;                  // read const by LiftChapter1Clear

// Ch1 加退選之亂 「善有善報」 quest flags — single source of truth, shared
// by the victim's-umbrella pickup table (ChapterQuestItems), the return
// hook below and DialogOpener's Ch1 victim routing so the names never
// drift (the kFlag* convention of Chapter2Quest.h / Chapter3Quest.h).
//
// Flag_PromisedVictim is set by the (b) 承諾 DialogChoice (chapter1.md 苦主
// (b), `Flag_PromisedVictim = true` / `// karma +5`, applied by
// ApplyDialogChoice), NOT here — it is the existing 承諾 beat and also
// drives the Ch1→Ch3→Ch4 ripple, so it lives with the dialogue. This
// header owns only the NEW reciprocity flag.
//
// Set by the QuestFlagPickup the player finds out in the world (the
// transparent umbrella the 西裝學長 dropped near 集英樓); read by
// TryReturnVictimUmbrella's GRANT phase. Mirrors Flag_HasSausage /
// Flag_FoundForm: a single-use quest item modelled as a flag, not the
// count-only consumable inventory.
inline constexpr const char* kFlagHasVictimUmbrella = "Flag_HasVictimUmbrella";

// T2 once-key: the Ch1 clear (UmbrellaClaimed publish → Interlude) has
// already fired. LiftChapter1Clear is polled every non-dialog frame, so
// this guard makes the publish happen exactly once after the (d) exchange
// dialogue closes. Mirrors Chapter2Quest's kFlagCh2Cleared once-guard.
inline constexpr const char* kFlagCh1ClearFired = "Flag_ClearChapter1";

// E-interact hook: returning the 苦主 (victim) his umbrella in Ch1. The
// reciprocity heart of the redesigned chapter — the chapter clears only
// AFTER the player carries the victim's umbrella BACK to him, never the
// instant an umbrella is grabbed off the ground. Sibling of
// TryRescueBookworm (Ch2) / TryAdvanceCh3Trade (Ch3); invoked from
// GameController's E-interact next to those hooks. No-op unless
// state==Chapter1_AddDrop && npcId=="victim".
//
//   DONE        — already holds Flag_HasTrueUmbrella: no-op (the grant
//                 already happened; a re-talk routes to the (d) recap).
//   NO PROMISE  — !Flag_PromisedVictim: no-op (the (a) plea / (b) 承諾
//                 DialogChoice handles the promise; nothing to return yet).
//   PROMISED,   — has the promise but NOT the victim's umbrella: a
//   NO UMBRELLA   ShowMessage reminder ("先幫他找回那把傘"). No state
//                 change — the player must still find the pickup.
//   GRANT       — has the promise AND Flag_HasVictimUmbrella: the payoff.
//                 Clears Flag_HasVictimUmbrella (the victim takes his傘
//                 back) + SetHasUmbrella(true) + SetFlag(Flag_HasTrueUmbrella)
//                 (Ending A's precise condition, EndingGate.cpp). T2: it
//                 does NOT publish UmbrellaClaimed here — that (which drives
//                 the Ch1→Interlude transition) is DEFERRED to
//                 LiftChapter1Clear so the (d) 重逢致謝 exchange dialogue
//                 plays FIRST. Idempotent: the grant clears nothing it
//                 re-reads, and the DONE guard above stops a second grant.
//
// The world TrueUmbrella is REMOVED in the redesign (the victim grants it
// now); the Cursed / Fragile / ProfessorTrap umbrellas stay on the ground
// as the morality / Ending-B paths and clear Ch1 via their own beClaimed —
// this hook is the ONLY non-cursed clear path.
void TryReturnVictimUmbrella(Player& player, std::string_view npcId,
                             SemesterState state);

// T2: deferred Ch1 clear, the sibling of Chapter2Quest's LiftChapter2Clear.
// Once TryReturnVictimUmbrella has granted Flag_HasTrueUmbrella, this fires
// the chapter clear — but only AFTER the 苦主's (d) 重逢致謝 exchange dialogue
// has CLOSED (the `dialog.Active()` guard), so the player reads the exchange
// scene before Ch1 snaps to the Interlude. Publishes ShowMessage THEN
// UmbrellaClaimed("TrueUmbrella") (beClaimed's HUD-slot order) and the
// EventWiring Ch1 sibling-if then transitions Ch1→Interlude (returnTo Ch2).
// Once-guarded (kFlagCh1ClearFired) so it fires exactly once though polled
// every non-dialog frame. No-op outside Ch1 / before the grant / while the
// dialogue is up / after it has already fired. Called by GameController next
// to LiftChapter2Clear, BEFORE CheckChapterGates, so the published
// transition is observed the same frame.
void LiftChapter1Clear(Player& player, SemesterState state,
                       const DialogState& dialog);

// G3: sequential `!` gate for the Ch1 MAIN spine — now a THREE-step
// sequence 苦主 → 西裝學長 → 苦主 (the owner wants 西裝學長 to light as the
// MIDDLE step). Mirrors the Ch2/Ch4 idiom (takes the roster isQuestGiver
// bit rather than the caller AND-ing it), because 西裝學長 ships
// isQuestGiver=false in DefaultNpcSpawns (NpcSpawns.h:46) yet MUST light at
// step 2. The sequence, keyed on the existing Ch1 flags:
//   step 1  — !Flag_PromisedVictim                : 苦主 lit (give the lead).
//   step 2  — Flag_PromisedVictim &&
//             !Flag_SuitSeniorChoiceMade           : 西裝學長 lit (confront /
//                                                    the choice).
//   step 3  — Flag_SuitSeniorChoiceMade &&
//             !Flag_HasTrueUmbrella                : 苦主 lit again (return
//                                                    his umbrella).
//   done    — Flag_HasTrueUmbrella                 : all dark (grant done;
//                                                    the (d) reunion is recap).
// Only the spine NPCs (victim / suit_senior) are sequenced here; every
// OTHER npc falls through to its roster isQuestGiver bit (the 助教 申請書
// errand / 學霸 / 阿姨 are isQuestGiver=false, so they never light as a main
// objective). Out-of-order CONTACT is redirected by the E-interact hooks
// (TryReturnVictimUmbrella ShowMessage nudge), not here — this only paints
// the `!`. View calls it (via QuestIndicatorVisible) when state ==
// Chapter1_AddDrop.
[[nodiscard]] bool Ch1IndicatorVisible(std::string_view npcId,
                                       bool isQuestGiver,
                                       const Player& player);

} // namespace nccu

#endif // CHAPTER1_QUEST_H_
