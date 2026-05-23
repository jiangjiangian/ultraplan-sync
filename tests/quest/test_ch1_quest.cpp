#include "doctest/doctest.h"
#include "quest/Chapter1Quest.h"
#include "quest/ChapterGate.h"
#include "quest/ChapterQuestItems.h"
#include "dialog/DialogState.h"
#include "controller/EventBus.h"
#include "controller/EventWiring.h"
#include "entities/Player.h"
#include "entities/QuestFlagPickup.h"
#include "entities/CursedUmbrella.h"
#include "state/SemesterStateMachine.h"
#include "gfx/Vec2.h"

#include <string>
#include <vector>

using nccu::SemesterState;

namespace {

Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }

// Captures every UmbrellaClaimed payload + the LAST ShowMessage. A scoped
// subscription keyed on free locals (the test_chapter_transitions idiom).
struct Capture {
    std::vector<std::string> umbrellaClaims;
    std::string              lastMessage;
    EventBus::Subscription   subUmb;
    EventBus::Subscription   subMsg;
};

[[nodiscard]] Capture MakeCapture() {
    Capture c;
    c.subUmb = EventBus::Instance().ScopedSubscribe(
        EventType::UmbrellaClaimed,
        [&c](const Event& e) { c.umbrellaClaims.push_back(e.text); });
    c.subMsg = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&c](const Event& e) { c.lastMessage = e.text; });
    return c;
}

}  // namespace

// REGRESSION 1 — the reciprocity grant. Returning the 苦主 his umbrella
// sets Flag_HasTrueUmbrella + publishes UmbrellaClaimed("TrueUmbrella")
// ONLY when the player BOTH promised AND holds the victim's umbrella; every
// other state (wrong state/npc, no promise, promised-but-empty-handed,
// already-granted) is a no-op for the grant.
TEST_CASE("TryReturnVictimUmbrella: grants the真傘 only after promise + return") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();

    // Wrong state / wrong npc -> no-op.
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter2_Midterms);
    nccu::TryReturnVictimUmbrella(p, "ta", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    CHECK(cap.umbrellaClaims.empty());

    // Promised? NO -> no-op (the (a)/(b) dialogue owns the promise; nothing
    // to return yet). No grant, no UmbrellaClaimed.
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    CHECK(cap.umbrellaClaims.empty());

    // Promised but WITHOUT the victim's umbrella -> reminder only, no grant.
    p.SetFlag("Flag_PromisedVictim");
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    CHECK_FALSE(p.HasUmbrella());
    CHECK(cap.umbrellaClaims.empty());
    CHECK_FALSE(cap.lastMessage.empty());           // a nudge was shown

    // Promised AND holding the victim's umbrella -> the GRANT fires.
    p.SetFlag(nccu::kFlagHasVictimUmbrella);
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));        // Ending A's condition
    CHECK(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasVictimUmbrella));  // 苦主 took his傘
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "TrueUmbrella");  // drives the Ch1 clear

    // Idempotent on a re-talk: already granted -> no second UmbrellaClaimed.
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// REGRESSION 1b — the grant must NOT fire if the player holds the victim's
// umbrella but never promised (defensive: both conditions are required).
TEST_CASE("TryReturnVictimUmbrella: umbrella without a promise never grants") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagHasVictimUmbrella);          // umbrella, but no promise
    nccu::TryReturnVictimUmbrella(p, "victim", SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    CHECK(cap.umbrellaClaims.empty());
    EventBus::Instance().Clear();
}

// REGRESSION 1c — the grant drives the existing Ch1→Interlude spine via the
// EventWiring UmbrellaClaimed sibling-if (returnTo Ch2), exactly as the
// removed world TrueUmbrella did.
TEST_CASE("Ch1 reciprocity grant reaches the Interlude via EventWiring") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    std::string buildingName;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, buildingName);
    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    Player p = MakePlayer();
    p.SetFlag("Flag_PromisedVictim");
    p.SetFlag(nccu::kFlagHasVictimUmbrella);
    nccu::TryReturnVictimUmbrella(p, "victim", m.Current());

    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter2_Midterms);
    EventBus::Instance().Clear();
}

// REGRESSION 2 — the three-ending architecture is untouched: a CursedUmbrella
// claim still works (its beClaimed sets Flag_TookCursedUmbrella + publishes
// UmbrellaClaimed → Ending B path), independent of the victim grant.
TEST_CASE("Ch1 morality umbrellas still claimable (Ending B path preserved)") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // Cursed claim is QuestGate-d on the promise (TransparentUmbrella), so
    // promise first, then claim (this mirrors the in-game gate).
    p.SetFlag("Flag_PromisedVictim");
    CursedUmbrella cursed{nccu::gfx::Vec2{0.0f, 0.0f}};
    cursed.beClaimed(&p);

    CHECK(p.HasUmbrella());
    CHECK(p.HasFlag("Flag_TookCursedUmbrella"));      // Ending B seed
    CHECK(p.GetKarma() == k0 - 30);                   // the -30 penalty
    REQUIRE(cap.umbrellaClaims.size() == 1);
    CHECK(cap.umbrellaClaims[0] == "CursedUmbrella"); // its own clear path
    // Idempotent (CLAUDE.md §5 / BUGLEDGER L2).
    cursed.beClaimed(&p);
    CHECK(cap.umbrellaClaims.size() == 1);
    EventBus::Instance().Clear();
}

// REGRESSION 3 — the findable victim's-umbrella pickup sets its flag (the
// QuestFlagPickup the player collects out in the world).
TEST_CASE("Ch1 victim's-umbrella pickup sets Flag_HasVictimUmbrella") {
    EventBus::Instance().Clear();
    Capture cap = MakeCapture();

    // It is the single Ch1 quest item, at (1450,1450), no completion set.
    const auto& ch1 = nccu::ChapterQuestItems(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 1);
    const auto& qi = ch1[0];
    CHECK(qi.flag == nccu::kFlagHasVictimUmbrella);

    Player p = MakePlayer();
    QuestFlagPickup pickup{qi.pos, qi.flag, qi.message, qi.completionFlags,
                           qi.completionKarma, qi.countMessages};
    pickup.OnPickup(&p);
    CHECK(p.HasFlag(nccu::kFlagHasVictimUmbrella));
    CHECK(cap.lastMessage == qi.message);             // its own pickup line
    EventBus::Instance().Clear();
}
