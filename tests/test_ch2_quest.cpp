#include "doctest/doctest.h"
#include "Chapter2Quest.h"
#include "ChapterGate.h"
#include "DialogOpener.h"
#include "DialogState.h"
#include "EventBus.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "gfx/Vec2.h"

using nccu::SemesterState;

namespace {

Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }

void GiveNotes(Player& p) {
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote2);
    p.SetFlag(nccu::kFlagFoundNote3);
}

}  // namespace

TEST_CASE("ResolveOpenerSubState: Ch2 librarian (b) gated on the 3 notes") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 0);
    GiveNotes(p);
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 1);
}

TEST_CASE("ResolveOpenerSubState: Ch2 bookworm (a) until recovered, then (d)") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 0);
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 3);

    // Ch1 routing is untouched (the new branch is Ch2-guarded).
    Player q = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter1_AddDrop, q) == 0);
}

TEST_CASE("TryRescueBookworm: needs 3 notes + EnergyDrink; rewards once") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();

    // Wrong state / wrong npc -> no-op.
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));
    nccu::TryRescueBookworm(p, "ta", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));

    // Notes missing -> no-op even holding a drink (not consumed).
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.ConsumableCount("EnergyDrink") == 1);

    // Notes in but the drink already spent -> hint only, nothing changes.
    GiveNotes(p);
    p.ConsumeOne("EnergyDrink");
    const int k0 = p.GetKarma();
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0);

    // Notes + drink -> consume, +5, recovered. Idempotent on a re-talk.
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0 + 5);
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.GetKarma() == k0 + 5);   // no double reward
    EventBus::Instance().Clear();
}

TEST_CASE("LiftChapter2Clear: deferred behind recovery + a closed dialog") {
    Player p = MakePlayer();
    nccu::DialogState d;

    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK_FALSE(p.HasFlag(nccu::kFlagCh2Cleared));   // not recovered yet

    p.SetFlag(nccu::kFlagBookwormRecovered);
    d.Open({"那個……算我欠你一個。"});
    REQUIRE(d.Active());
    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK_FALSE(p.HasFlag(nccu::kFlagCh2Cleared));   // (d) still on screen

    d.Close();
    nccu::LiftChapter2Clear(p, SemesterState::Chapter2_Midterms, d);
    CHECK(p.HasFlag(nccu::kFlagCh2Cleared));          // lifted

    // Wrong state never lifts.
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagBookwormRecovered);
    nccu::LiftChapter2Clear(q, SemesterState::Chapter3_SportsDay, d);
    CHECK_FALSE(q.HasFlag(nccu::kFlagCh2Cleared));
}

TEST_CASE("Ch2 quest reaches the Interlude via the existing spine") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    Player p = MakePlayer();
    nccu::DialogState d;
    m.Transition(SemesterState::Chapter2_Midterms);

    GiveNotes(p);
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(p, "bookworm", m.Current());
    nccu::LiftChapter2Clear(p, m.Current(), d);       // dialog closed
    REQUIRE(p.HasFlag(nccu::kFlagCh2Cleared));

    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);
    EventBus::Instance().Clear();
}
