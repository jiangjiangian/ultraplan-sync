// Items 1b + 4a + 4b regression for the quest `!` decision layer.
//
// QuestIndicatorVisible is the single source of truth the View consults
// (ui/View.cpp) to decide whether an NPC paints a quest `!`. It folds the
// roster's isQuestGiver bit together with the per-chapter rules:
//   • Ch4 (Item 1b): the gate-driven roster ships every NPC isQuestGiver
//     =false, so pre-fix Ch4 drew NO `!` at all and the finale had no
//     visual cue. Ch4IndicatorVisible lights 助教 ONLY, until the (d) 結算
//     choice is locked in (Flag_TaFinaleChoiceMade).
//   • Ch3 (Item 4a): A is the chain head from chapter entry (pre-lap), so
//     the first step is always discoverable.
//   • Ch3 (Item 4b): the 5 archetypes are isQuestGiver=false in the Ch3
//     roster, so they never light alongside the A→B→C chain — confirmed
//     here by feeding isQuestGiver=false through the predicate.
//
// Revert-verify: see the inline notes per case.

#include "doctest/doctest.h"
#include "quest/QuestIndicator.h"
#include "quest/Chapter4Quest.h"
#include "quest/Chapter3Quest.h"
#include "entities/Player.h"
#include "gfx/Vec2.h"

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh3 = SemesterState::Chapter3_SportsDay;
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
constexpr auto kCh1 = SemesterState::Chapter1_AddDrop;
}  // namespace

TEST_CASE("Ch4IndicatorVisible: 助教 is the finale `!` until the choice is made") {
    Player p = MakePlayer();
    // 助教 lit on entry — the one NPC that advances Ending A/B/C.
    CHECK(nccu::Ch4IndicatorVisible("ta", p));
    // Every other Ch4 archetype stays dark (the finale is gate-driven).
    CHECK_FALSE(nccu::Ch4IndicatorVisible("victim", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("suit_senior", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("bookworm", p));
    CHECK_FALSE(nccu::Ch4IndicatorVisible("shop_auntie", p));
    // Once the 結算 choice is committed, even 助教 goes dark (resolved).
    p.SetFlag("Flag_TaFinaleChoiceMade");
    CHECK_FALSE(nccu::Ch4IndicatorVisible("ta", p));
}

TEST_CASE("QuestIndicatorVisible Ch4: 助教 lights regardless of roster bit") {
    // Item 1b core: the Ch4 roster marks 助教 isQuestGiver=FALSE, yet the
    // finale `!` must still light — the predicate keys on npcId, not the
    // bit. Revert-verify: make the Ch4 branch fall through to the default
    // `return isQuestGiver` and this CHECK fails (no `!` in Ch4).
    Player p = MakePlayer();
    CHECK(nccu::QuestIndicatorVisible("ta", /*isQuestGiver=*/false, kCh4, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/false, kCh4, p));
    // An empty-id object (Player / item / ambient student) never lights.
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("", /*isQuestGiver=*/false, kCh4, p));
    p.SetFlag("Flag_TaFinaleChoiceMade");
    CHECK_FALSE(nccu::QuestIndicatorVisible("ta", false, kCh4, p));
}

TEST_CASE("QuestIndicatorVisible Ch3: chain head lit pre-lap, archetypes dark") {
    Player p = MakePlayer();
    // Item 4a: A (a quest-giver) lit from entry, pre-lap.
    CHECK(nccu::QuestIndicatorVisible("vendor_sausage_a",
                                      /*isQuestGiver=*/true, kCh3, p));
    // Item 4b: a repositioned archetype is isQuestGiver=false in the Ch3
    // roster, so it stays dark even though it shares the chapter with the
    // active chain — no stray `!`. (The && in QuestIndicatorVisible's Ch3
    // branch enforces this; revert-verify: drop the isQuestGiver factor and
    // the archetype would inherit Ch3IndicatorVisible's `return true`.)
    CHECK_FALSE(nccu::QuestIndicatorVisible("ta",
                                            /*isQuestGiver=*/false, kCh3, p));
    CHECK_FALSE(nccu::QuestIndicatorVisible("suit_senior",
                                            /*isQuestGiver=*/false, kCh3, p));
    // B/C links stay dark until their turn.
    CHECK_FALSE(nccu::QuestIndicatorVisible("loudspeaker_b", true, kCh3, p));
    CHECK_FALSE(nccu::QuestIndicatorVisible("senior_c", true, kCh3, p));
}

TEST_CASE("QuestIndicatorVisible default: Ch1/Ch2 pass through isQuestGiver") {
    // Outside Ch3/Ch4 the predicate is a pure pass-through of the roster
    // bit — Ch1 苦主 (quest-giver) lit, a non-quest-giver dark — so existing
    // chapters are byte-unchanged.
    Player p = MakePlayer();
    CHECK(nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh1, p));
}
