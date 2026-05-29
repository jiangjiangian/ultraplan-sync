#ifndef CHAPTER2_QUEST_H_
#define CHAPTER2_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the rescue / lift
class EventBus;                     // Plan P2 step 2: bus is injected

namespace nccu {

class DialogState;                  // queried (Active) by the lift

// Ch2 圖書館期中考 quest flag USAGE notes. The `kFlag*` constants live in
// `quest/Flags.h`; the per-flag semantics that follow describe how Ch2's
// rescue/lift/ripple machinery wires them.
//
//   kFlagFoundNote1/2/3 — set by the note pickups (ChapterQuestItems).
//   kFlagBookworm       — 學霸 has been woken with an EnergyDrink and asked
//     the player to pick up his scattered notes — i.e. the note quest has
//     STARTED. Gates the note spawn (World::MaybeSpawnChapter2Notes), the
//     bookworm dialog routing and the rescue's second phase. Set ONLY by
//     TryRescueBookworm's wake step, never by content.
//   kFlagMetLibrarian   — A2 (hard-gate the Ch2 spine 管理員 → 學霸 → 撿筆記 →
//     學霸): set the FIRST time the player talks to the 圖書館管理員 in Ch2
//     (TryMeetLibrarian, run at the E-interact moment her clue line opens).
//     Gates BOTH the 學霸 dialog (DialogOpener routes him to a "先去問櫃台的
//     管理員" redirect until it is set) AND TryRescueBookworm's wake step.
//     Idempotent (SetFlag is a no-op once held); set only by TryMeetLibrarian.
//   kFlagLibrarianUmbrella — B2.3 once-key: the 圖書館管理員 has already lent
//     the player her 折疊傘 (管理員的傘). Set by TryLendLibrarianUmbrella so
//     a re-talk to the librarian does NOT re-grant / stack the loaner. NOT
//     an ending flag.
//   kFlagLibrarianUmbrellaReturned — G-3 once-key: the player has RETURNED
//     管理員的傘 at the 中正圖書館 front in the Ch2→Ch3 Interlude (the
//     optional 責任感 beat). Set by TryReturnLibrarianUmbrella so the +10
//     karma grant + loaner clear fire EXACTLY ONCE; NOT an ending flag.
//   kFlagCh2RippledSuitSenior / kFlagCh2RippledTA — S5c-3 per-NPC "the Ch2
//     ripple karma already landed" once-keys. The ripple dialog itself may
//     re-show on re-talks; only the karma adjustment is gated to fire
//     exactly once per Ch2.

// G-3: the Interlude return-point marker's npcId (a small NPC at the library
// front the player presses E on to return the loaner). Single source so the
// spawn, the hook and the dialog routing never drift.
inline constexpr const char* kNpcLibrarianReturn = "librarian_return";

// All three 學霸 notes collected — gates 圖書館管理員 (b) and the exchange.
[[nodiscard]] bool Chapter2NotesComplete(const Player& player);

// E-interact hook: talking to 學霸 (bookworm). A two-phase quest machine
// keyed on kFlagBookworm. No-op unless state==Chapter2_Midterms,
// npcId=="bookworm", and 學霸 is not yet recovered.
//
//   PHASE 1 — ASLEEP (no wake flag): the 學霸 is slumped at the 羅馬廣場
//     statue. With an EnergyDrink in the count-inventory it is CONSUMED
//     here -> Flag_Bookworm set + the 學霸 wakes and asks for his notes
//     (this is the moment the note quest starts; World then spawns the 3
//     notes). Without a drink, a hint to the 圖書館地下室自販機 fallback
//     (35 元) is shown and nothing is consumed.
//   PHASE 2 — WOKEN (wake flag set): if the 3 notes are NOT all in, a
//     reminder line is shown. Once all 3 are in, the exchange fires
//     (notes <-> the player's umbrella): Flag_BookwormRecovered + karma
//     +5 (chapter2.md 學霸 (d) `// karma +5`; path-b — the (d) blockquote
//     carries no flag annotation, so the opener's once-apply never fires
//     it). The EnergyDrink is spent at the WAKE step, NOT here.
//
// Deliberately does NOT set Flag_Ch2Cleared — that is lifted later so the
// (d) thanks dialog is readable first (see LiftChapter2Clear).
// Plan P2 step 2: `bus` is injected; this hook publishes ShowMessage
// for every wake / recovery / hint beat.
void TryRescueBookworm(EventBus& bus, Player& player,
                       std::string_view npcId, SemesterState state);

// Deferred chapter-clear lift. Sets Flag_Ch2Cleared only once 學霸 is
// recovered AND no dialog is open — i.e. after the player has finished
// reading the (d) thanks. CheckChapterGates' existing Ch2 sibling-if
// then transitions to the Interlude. Decoupling the clear from the
// rescue interact stops the gate from closing the (d) dialog on the
// same frame it opens (mirrors Flag_LeaveInterlude: trigger, then a
// polled consume).
void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog);

// A2 — the 圖書館管理員 is the Ch2 chain head. E-interact hook, sibling of
// TryRescueBookworm; called from GameController next to it, BEFORE the opener
// runs. Sets kFlagMetLibrarian the first time the player talks to her in Ch2
// (her (a) clue line points to 羅馬廣場 / the 學霸), which UNLOCKS the 學霸:
// before it is set, DialogOpener routes the 學霸 to a "先去問櫃台的管理員"
// redirect and TryRescueBookworm refuses to wake him. No-op unless
// state==Chapter2_Midterms && npcId=="librarian". Idempotent (SetFlag is a
// no-op once held), so a re-talk is harmless.
void TryMeetLibrarian(Player& player, std::string_view npcId,
                      SemesterState state);

// B2.3 — the 圖書館管理員 lends the player her 折疊傘 (管理員的傘). E-interact
// hook, sibling of TryRescueBookworm; called from GameController next to it.
// chapter2.md 管理員 (b) already speaks the hand-over ("（遞過一把折疊傘）這個
// 先拿著，別在外面淋著。") and the 補設定 (L356/L397) specifies a loaner that
// shelters but still accrues rain — this wires that beat in engine. No-op
// unless state==Chapter2_Midterms && npcId=="librarian". Gated on
// Flag_Bookworm (the (b) state: the player has woken the 學霸 and come back),
// so the loaner arrives exactly when her (b) line plays. Idempotent via
// kFlagLibrarianUmbrella so a re-talk never stacks umbrellas.
//
// Effect: SetHeldUmbrella(HeldUmbrella::Loaner) — the player now HOLDS
// 管理員的傘 (a bag umbrella row whose description says it auto-reduces rain
// while held) and HasUmbrella() becomes true so the existing
// outdoors+umbrella ApplyRainSheltered drain kicks in. It does NOT set
// Flag_HasTrueUmbrella — the loaner is explicitly NOT the true umbrella, so
// it can never by itself satisfy Ending A's 持-TrueUmbrella condition.
void TryLendLibrarianUmbrella(Player& player, std::string_view npcId,
                              SemesterState state);

// G-3 — return 管理員的傘 in the Ch2→Ch3 Interlude (the optional 責任感
// beat). E-interact hook, sibling of TryLendLibrarianUmbrella; called from
// GameController next to the other hooks, BEFORE the opener runs. The
// return-point marker (kNpcLibrarianReturn) is deferred-spawned by
// World::MaybeSpawnInterludeLibrarianReturn at the 中正圖書館 front ONLY while
// the player still holds the loaner in the market that returns to Ch3, so by
// the time this hook can fire those preconditions already hold; the hook
// re-checks them anyway (defensive + the one place state mutates).
//
// `returnTo` is World::Semester().InterludeReturnTo() — the gate that scopes
// this to the Ch2→Ch3 market (== Chapter3_SportsDay) and NOTHING else.
//
//   WRONG CONTEXT — not the Interlude, not the Ch2→Ch3 market, or not the
//                   return-point npcId: no-op.
//   ALREADY DONE  — kFlagLibrarianUmbrellaReturned set: replay a short
//                   closure ShowMessage (re-talk friendly), NO second karma.
//   RETURN        — still holds the loaner (Flag_LibrarianUmbrella +
//                   HeldUmbrella::Loaner): karma +10, clear the held umbrella
//                   (SetHasUmbrella(false)) + Flag_LibrarianUmbrella, set the
//                   returned once-key, ShowMessage a thank-you. Grants NO
//                   ending-affecting umbrella flag (the loaner was never the
//                   true umbrella). If the player skips this, the loaner still
//                   auto-clears on Ch3 entry (no karma) — so this is purely a
//                   positive optional choice, never a soft-lock.
// Plan P2 step 2: `bus` is injected; publishes the 責任感 +10 thank-you
// / replay ShowMessage.
void TryReturnLibrarianUmbrella(EventBus& bus, Player& player,
                                std::string_view npcId, SemesterState state,
                                SemesterState returnTo);

// E-interact hook, sibling of TryRescueBookworm — lands the Ch1->Ch2
// ripple karma the dialog opener cannot. chapter2.md routes 西裝學長 /
// 助教 to a flag-gated subState whose `// karma` the opener's once-apply
// will NOT grant (it is guarded behind a NOT-yet-set flag, but the
// ripple flags are Ch1 flags already held — or the entry is karma-only
// with no flag). So the documented "karma -10 在此落地" / ±3 must be
// applied here, once per Ch2 (per-NPC once-key). No-op for any other
// NPC / state / flag combination. Precedence (chapter2.md L211/L225):
//   助教   HasProfessorTrap (c, 取代 a/b, -10)  >  HelpedTA_Ch1 (b, 0)
//   西裝學長 HelpedSenior (b, +3)  xor  ScoldedSenior (c, -3)
void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

// T3: sequential quest-giver `!` gate for the Ch2 MAIN spine — the sibling
// of Ch3IndicatorVisible. Spine: 圖書館管理員(線索) → 學霸(喚醒) →
// [find the 3 notes] → 學霸(換回). The `!` advances by main-quest order:
//   • 圖書館管理員 (librarian): the chain head, lit from chapter entry until
//     the 學霸 is woken (kFlagBookworm). She gives the clue pointing to
//     羅馬廣場; once the 學霸 is up, her job is done and she goes dark.
//   • 學霸 (bookworm): lit ONLY after he is woken, and stays lit through the
//     note hunt + the 換回 return, until Flag_BookwormRecovered. Before he is
//     woken he is DARK (the `!` guides the player to the librarian first),
//     exactly like Ch3's B stays dark until A is traded.
// Keyed purely on npcId (NOT the roster bit): the Ch2 roster ships 學霸 as
// isQuestGiver=false, so QuestIndicatorVisible must consult this WITHOUT
// AND-ing isQuestGiver (mirrors the Ch4 finale gate) — otherwise 學霸 could
// never light. Every other npcId returns its isQuestGiver bit so non-spine
// NPCs are unaffected. View calls this when state == Chapter2_Midterms.
[[nodiscard]] bool Ch2IndicatorVisible(std::string_view npcId,
                                       bool isQuestGiver,
                                       const Player& player);

} // namespace nccu

#endif // CHAPTER2_QUEST_H_
