#ifndef CHAPTER1_QUEST_H_
#define CHAPTER1_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the return / grant
class EventBus;                     // Plan P2 step 2: bus is injected

namespace nccu {

class DialogState;                  // read const by LiftChapter1Clear

// Ch1 加退選之亂 「善有善報」 quest flag usage. The actual `kFlag*` constants
// live in `quest/Flags.h` (single source of truth for every `Flag_*` string).
// This header documents how Ch1 USES them — Flag_PromisedVictim is set by
// the (b) 承諾 DialogChoice in chapter1.md (`Flag_PromisedVictim = true` /
// `// karma +5`, applied by ApplyDialogChoice); kFlagHasVictimUmbrella is
// set by the QuestFlagPickup the player finds near 集英樓 (mirrors
// Flag_HasSausage / Flag_FoundForm: a single-use quest item modelled as a
// flag, not the count-only consumable inventory) and read by
// TryReturnVictimUmbrella's GRANT phase. kFlagClearChapter1 is the T2
// once-key: the Ch1 clear (UmbrellaClaimed publish → Interlude) has already
// fired, polled by LiftChapter1Clear so the publish happens exactly once
// after the (d) exchange dialogue closes.

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
// as the morality / Ending-B paths and clear Ch1 via their own BeClaimed —
// this hook is the ONLY non-cursed clear path.
// Plan P2 step 2: `bus` is injected; this hook publishes ShowMessage
// when the player meets the victim without yet having his umbrella.
void TryReturnVictimUmbrella(EventBus& bus, Player& player,
                             std::string_view npcId, SemesterState state);

// T2: deferred Ch1 clear, the sibling of Chapter2Quest's LiftChapter2Clear.
// Once TryReturnVictimUmbrella has granted Flag_HasTrueUmbrella, this fires
// the chapter clear — but only AFTER the 苦主's (d) 重逢致謝 exchange dialogue
// has CLOSED (the `dialog.Active()` guard), so the player reads the exchange
// scene before Ch1 snaps to the Interlude. Publishes ShowMessage THEN
// UmbrellaClaimed("TrueUmbrella") (BeClaimed's HUD-slot order) and the
// EventWiring Ch1 sibling-if then transitions Ch1→Interlude (returnTo Ch2).
// Once-guarded (kFlagClearChapter1) so it fires exactly once though polled
// every non-dialog frame. No-op outside Ch1 / before the grant / while the
// dialogue is up / after it has already fired. Called by GameController next
// to LiftChapter2Clear, BEFORE CheckChapterGates, so the published
// transition is observed the same frame.
// Plan P2 step 2: `bus` is injected; this poll publishes the
// ShowMessage + UmbrellaClaimed pair that drives Ch1→Interlude.
void LiftChapter1Clear(EventBus& bus, Player& player, SemesterState state,
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

// B3: the Ch1 樂活小舖 醜綠傘 is a REAL purchase, not a narrative seed. The
// price the 阿姨 quotes in chapter1.md 苦主 (b) (八十塊) — single source so
// the toast and the deduction can't drift.
inline constexpr int kCh1UglyUmbrellaPrice = 80;

// B3: E-interact / dialog-confirm hook — buying the 阿姨's 醜綠傘 in Ch1.
// chapter1.md 福利社阿姨 (c) 購買醜綠傘 used to be a pure "narrative seed"
// (annotated `// karma +0`, no money, no umbrella). It is now a genuine
// transaction: deduct kCh1UglyUmbrellaPrice and grant the HELD ugly umbrella
// (SetHeldUmbrella(HeldUmbrella::Ugly) → a bag row + auto rain shelter), with
// a visible 花費/餘額 toast (the Vendor::TryBuy idiom, reusing the same
// vendor::msg copy). Called by GameController when the 阿姨's 購買醜綠傘
// choice is confirmed. No-op unless state==Chapter1_AddDrop && npcId==
// "shop_auntie" && the chosen label is the buy label.
//
//   ALREADY OWN  — already holds HeldUmbrella::Ugly: no-op (idempotent;
//                  the 阿姨's menu is re-presented, so without this a
//                  re-pick would re-deduct 80 for an umbrella in hand).
//   TOO POOR     — DeductMoney(price) fails: ShowMessage "你錢不夠", purse
//                  and held umbrella both untouched (the purchase failed).
//   BUY          — funds enough: deduct, SetHeldUmbrella(Ugly), 花費/餘額
//                  toast. CRUCIALLY does NOT set Flag_BoughtUglyUmbrella —
//                  that is the Ch4 集英樓 Vendor's Ending-C commitment, NOT
//                  this in-chapter umbrella (EndingGate.cpp). No karma.
//
// Returns true iff a purchase actually completed (for the test / caller),
// false on every no-op / decline / poor path.
// Plan P2 step 2: `bus` is injected; this dialog-confirm hook publishes
// the 花費/餘額 / 你錢不夠 toast.
bool TryBuyAuntieUglyUmbrella(EventBus& bus, Player& player,
                              std::string_view npcId,
                              std::string_view choiceLabel,
                              SemesterState state);

} // namespace nccu

#endif // CHAPTER1_QUEST_H_
