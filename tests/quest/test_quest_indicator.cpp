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
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
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
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;
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

// T3: Ch1 single-NPC spine — the 苦主's `!` rides the chapter and goes dark
// once the grant fires. The 助教 side errand (isQuestGiver=false) never lights.
TEST_CASE("T3: QuestIndicatorVisible Ch1 sequences the 苦主 `!`") {
    Player p = MakePlayer();
    // 苦主 lit from entry (the承諾 → 找傘 → 歸還 target).
    CHECK(nccu::Ch1IndicatorVisible("victim", p));
    CHECK(nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    // The grant (Flag_HasTrueUmbrella) completes the objective -> dark.
    p.SetFlag("Flag_HasTrueUmbrella");
    CHECK_FALSE(nccu::Ch1IndicatorVisible("victim", p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("victim", /*isQuestGiver=*/true, kCh1, p));
    // The 助教 errand is isQuestGiver=false, so it is never a main `!`.
    Player q = MakePlayer();
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("ta", /*isQuestGiver=*/false, kCh1, q));
}

// T3: Ch2 spine 圖書館管理員 → 學霸 sequences by main-quest order, and 學霸
// lights despite shipping isQuestGiver=false in the Ch2 roster.
TEST_CASE("T3: QuestIndicatorVisible Ch2 sequences 管理員 -> 學霸") {
    Player p = MakePlayer();
    // Entry: 管理員 (chain head) lit, 學霸 dark (guides to the librarian).
    CHECK(nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // Wake the 學霸: the head goes dark, the `!` moves to 學霸 — even though
    // his roster bit is FALSE (revert-verify: AND isQuestGiver in the Ch2
    // branch and this CHECK fails — 學霸 could never light).
    p.SetFlag(nccu::kFlagBookwormWoken);
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
    CHECK(nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // 學霸 stays lit through the note hunt + the 換回 return…
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote2);
    p.SetFlag(nccu::kFlagFoundNote3);
    CHECK(nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));

    // …until recovered (換回 done) -> dark.
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("bookworm", /*isQuestGiver=*/false, kCh2, p));
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("librarian", /*isQuestGiver=*/true, kCh2, p));
}

// A non-spine Ch2 NPC keeps its roster bit (the gate only special-cases the
// two spine NPCs; everything else passes isQuestGiver straight through).
TEST_CASE("T3: Ch2 non-spine NPC keeps its isQuestGiver bit") {
    Player p = MakePlayer();
    CHECK(nccu::Ch2IndicatorVisible("suit_senior", /*isQuestGiver=*/true, p));
    CHECK_FALSE(nccu::Ch2IndicatorVisible("suit_senior", /*isQuestGiver=*/false, p));
    // An empty-id object (player / item / ambient) never lights.
    CHECK_FALSE(
        nccu::QuestIndicatorVisible("", /*isQuestGiver=*/false, kCh2, p));
}
