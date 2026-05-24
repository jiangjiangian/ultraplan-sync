#include "doctest/doctest.h"
#include "quest/Chapter2Quest.h"
#include "quest/ChapterGate.h"
#include "quest/ItemCatalog.h"
#include "dialog/DialogOpener.h"
#include "dialog/DialogState.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include "state/SemesterStateMachine.h"
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

TEST_CASE("ResolveOpenerSubState: Ch2 librarian (b) gated on the wake flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 0);
    // The librarian now points to the 學霸; (b) confirms once he is woken,
    // NOT once the notes are in (notes only exist after waking).
    p.SetFlag(nccu::kFlagBookwormWoken);
    CHECK(nccu::ResolveOpenerSubState(
              "librarian", SemesterState::Chapter2_Midterms, p) == 1);
}

TEST_CASE("ResolveOpenerSubState: Ch2 bookworm (a)->(c) woken->(d) recovered") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 0);  // (a)
    p.SetFlag(nccu::kFlagBookwormWoken);
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 2);  // (c)
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, p) == 3);  // (d)

    // Ch1 routing is untouched (the new branch is Ch2-guarded).
    Player q = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter1_AddDrop, q) == 0);
}

TEST_CASE("TryRescueBookworm: wake step consumes drink; exchange needs notes") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();

    // Wrong state / wrong npc -> no-op.
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormWoken));
    nccu::TryRescueBookworm(p, "ta", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormWoken));

    // PHASE 1, asleep with NO drink -> hint only, nothing set/consumed.
    const int k0 = p.GetKarma();
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormWoken));
    CHECK(p.GetKarma() == k0);

    // PHASE 1, asleep WITH a drink -> consume + wake. NOT yet recovered,
    // and notes are irrelevant here (waking is what starts the note quest).
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookwormWoken));
    CHECK(p.ConsumableCount("EnergyDrink") == 0);   // spent at the wake step
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0);                       // +5 is at the exchange

    // PHASE 2, woken but notes incomplete -> reminder only, no recovery,
    // and crucially NO further drink is needed/consumed.
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));

    // PHASE 2, woken + all 3 notes -> exchange: +5, recovered, no drink
    // consumed (none held). Idempotent on a re-talk.
    GiveNotes(p);
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookwormRecovered));
    CHECK(p.GetKarma() == k0 + 5);
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.GetKarma() == k0 + 5);   // no double reward
    EventBus::Instance().Clear();
}

// B2.2: the 提神飲料 is HANDED OVER to the 學霸 — the bag count drops by
// EXACTLY ONE (not flag-checked, not a wipe-all). Holding two proves a single
// decrement.
TEST_CASE("B2.2: waking the 學霸 spends exactly one 提神飲料 from the bag") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.AddConsumable("EnergyDrink").AddConsumable("EnergyDrink");   // hold 2
    CHECK(p.ConsumableCount("EnergyDrink") == 2);

    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.HasFlag(nccu::kFlagBookwormWoken));
    CHECK(p.ConsumableCount("EnergyDrink") == 1);   // 2 -> 1, exactly one given

    // A second talk (now woken, notes incomplete) must NOT spend the other.
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK(p.ConsumableCount("EnergyDrink") == 1);   // no extra decrement
    EventBus::Instance().Clear();
}

// B2.3: the 圖書館管理員 lends 管理員的傘 once the 學霸 is woken (her (b)
// state). The player then HOLDS the loaner (a bag umbrella row + auto-shelter)
// — but it is NOT the true umbrella, so Ending A's Flag_HasTrueUmbrella stays
// unset. Idempotent: a re-talk never stacks umbrellas.
TEST_CASE("B2.3: 圖書館管理員 lends 管理員的傘 (held + shelter, NOT the true umbrella)") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();

    // Wrong state / wrong npc -> no grant.
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter1_AddDrop);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
    nccu::TryLendLibrarianUmbrella(p, "ta", SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);

    // Before the 學霸 is woken (her (a) clue state) -> no loaner yet.
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
    CHECK_FALSE(p.HasUmbrella());

    // Woken -> her (b) hand-over: the player now holds 管理員的傘.
    p.SetFlag(nccu::kFlagBookwormWoken);
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    CHECK(p.HasUmbrella());                              // auto-shelter armed
    CHECK(p.HasFlag(nccu::kFlagLibrarianUmbrellaLent));
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));      // NOT the true umbrella

    // The bag shows ONE 管理員的傘 row, no true-umbrella row.
    const auto rows = nccu::BuildInventoryRows(p);
    int loanerRows = 0;
    for (const auto& r : rows)
        if (r.itemId == nccu::kItemLoanerUmbrella) ++loanerRows;
    CHECK(loanerRows == 1);

    // Idempotent: a re-talk does not stack / re-grant.
    nccu::TryLendLibrarianUmbrella(p, "librarian",
                                   SemesterState::Chapter2_Midterms);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    EventBus::Instance().Clear();
}

TEST_CASE("TryRescueBookworm: notes BEFORE waking never trigger the exchange") {
    // Gating regression: even holding all 3 notes, an ASLEEP 學霸 (no wake
    // flag) cannot be exchanged. Without a drink the first talk only hints;
    // recovery is impossible until the wake flag is set. (In production the
    // notes cannot even exist pre-wake — World gates their spawn — but the
    // quest logic itself must also refuse the exchange.)
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    GiveNotes(p);                              // notes somehow in hand
    nccu::TryRescueBookworm(p, "bookworm", SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormRecovered));  // asleep -> no go
    CHECK_FALSE(p.HasFlag(nccu::kFlagBookwormWoken));      // no drink -> asleep
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

    // New flow: wake first (drink), THEN collect notes, THEN exchange.
    p.AddConsumable("EnergyDrink");
    nccu::TryRescueBookworm(p, "bookworm", m.Current());   // wake (phase 1)
    REQUIRE(p.HasFlag(nccu::kFlagBookwormWoken));
    GiveNotes(p);
    nccu::TryRescueBookworm(p, "bookworm", m.Current());   // exchange (phase 2)
    REQUIRE(p.HasFlag(nccu::kFlagBookwormRecovered));
    nccu::LiftChapter2Clear(p, m.Current(), d);       // dialog closed
    REQUIRE(p.HasFlag(nccu::kFlagCh2Cleared));

    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);
    EventBus::Instance().Clear();
}
