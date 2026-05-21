#include "doctest/doctest.h"
#include "ChapterToast.h"
#include "EventBus.h"
#include "EventWiring.h"
#include "InterludeExit.h"
#include "Player.h"
#include "SceneRouter.h"
#include "World.h"
#include "gfx/Vec2.h"

#include <string>
#include <string_view>

using nccu::SceneRouter;
using nccu::SemesterState;
using nccu::World;

// Cycle 10.P0a (awsome_cpp.md §6): SceneRouter owns the chapter/
// interlude/ending transition observer that used to live inline on
// GameController. These tests pin the contract:
//   1. ctor cursor initialises from the World's current FSM state.
//   2. Settle is a no-op when the FSM hasn't moved (idempotent).
//   3. Settle on a state change actually respawns the NPCs.
//   4. Settle on Interlude entry moves the player to kInterludeEntry,
//      wipes consumables, publishes the arrival hint, resets the
//      south-band latch.
//   5. Settle on Ch4 entry clears the umbrella state per chapter4.md L6.

namespace {

bool HasNpcId(const World& w, const char* id) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(id)) return true;
    return false;
}

// Latest ShowMessage payload — used to verify the arrival hint
// publishes (or doesn't) on the right call.
[[nodiscard]] EventBus::Subscription
SubscribeToLatest(std::string& latest) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&latest](const Event& e) { latest = e.text; });
}

} // namespace

TEST_CASE("SceneRouter ctor: cursor + latch start at the initial state") {
    SceneRouter r{SemesterState::Chapter1_AddDrop};
    CHECK(r.LastRosterState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.InterludeExitLatchMut() == false);
}

TEST_CASE("Settle: no-op when the FSM hasn't moved") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 roster present

    r.Settle(w);                           // FSM still Ch1
    CHECK(HasNpcId(w, "victim"));          // unchanged
    CHECK(r.LastRosterState() == SemesterState::Chapter1_AddDrop);
}

TEST_CASE("Settle: respawns the new chapter's NPCs on a transition") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPC

    // FSM advances WITHOUT going through the SceneRouter (mimics a
    // CheckChapterGates / EventWiring transition firing during the
    // controller's Update body).
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.Settle(w);

    // Ch2 roster now visible on the View / state.jsonl. Cursor stamped.
    CHECK(HasNpcId(w, "librarian"));       // Ch2-only quest-giver
    CHECK(HasNpcId(w, "victim"));          // victim is in Ch2 too
    CHECK(r.LastRosterState() == SemesterState::Chapter2_Midterms);
}

TEST_CASE("Settle Interlude entry: pos, consumables, hint, latch") {
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Pre-state: pretend the player wandered far from the IL entry,
    // carries a Ch1 consumable, and the south-band latch fired earlier.
    p->SetPosition(nccu::gfx::Vec2{100.0f, 100.0f});
    p->AddConsumable("EnergyDrink");
    p->AddConsumable("EnergyDrink");
    r.InterludeExitLatchMut() = true;
    REQUIRE(p->ConsumableCount("EnergyDrink") == 2);

    // FSM advances to Interlude (mimics the EventWiring Ch1→IL hop).
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.Settle(w);

    // All four observable side effects fire together:
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // wiped
    CHECK(latestHud == nccu::kInterludeArrivalHint);
    CHECK(r.InterludeExitLatchMut() == false);

    // Cursor stamped so a re-call is a no-op.
    CHECK(r.LastRosterState() == SemesterState::Interlude_Market);
    p->SetPosition(nccu::gfx::Vec2{42.0f, 42.0f});
    latestHud.clear();
    r.Settle(w);                                       // idempotent
    CHECK(p->GetPosition().x == doctest::Approx(42.0f));
    CHECK(latestHud.empty());

    EventBus::Instance().Clear();
}

TEST_CASE("Settle Ch4 entry: umbrella + TrueUmbrella flag reset") {
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetHasUmbrella(true);
    p->SetFlag("Flag_HasTrueUmbrella");

    // FSM jumps straight to Ch4 (mimics ChapterGate Interlude→Ch4).
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    r.Settle(w);

    // chapter4.md L6「傘再度失蹤」: the player walks out of 集英樓 with
    // no umbrella. The fix's Ch4 branch is the GDD enforcement.
    CHECK_FALSE(p->HasUmbrella());
    CHECK_FALSE(p->HasFlag("Flag_HasTrueUmbrella"));
    CHECK(r.LastRosterState() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

TEST_CASE("Settle: full spine traversal — every step's roster lands") {
    // Walk the seven-transition spine — Ch1 -> Interlude -> Ch2 ->
    // Interlude -> Ch3 -> Interlude -> Ch4 -> Ending_A — calling
    // Settle at every step. Each step's destination state must be
    // observable through the cursor stamp + the roster swap.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    const SemesterState path[] = {
        SemesterState::Interlude_Market,
        SemesterState::Chapter2_Midterms,
        SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay,
        SemesterState::Interlude_Market,
        SemesterState::Chapter4_Finals,
        SemesterState::Ending_A,
    };

    for (SemesterState s : path) {
        w.Semester().Transition(s);
        r.Settle(w);
        CHECK(r.LastRosterState() == s);
    }

    EventBus::Instance().Clear();
}

TEST_CASE("Settle: latch reset is once-per-Interlude-entry, not every call") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    // First Interlude entry: latch is reset to false. Caller flips it
    // true (mirrors the production path's MaybeAnnounceInterludeExit).
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.Settle(w);
    REQUIRE(r.InterludeExitLatchMut() == false);
    r.InterludeExitLatchMut() = true;

    // Settle again while still in the Interlude — no-op, latch stays
    // true (the player can't be re-bounced by repeated Settles).
    r.Settle(w);
    CHECK(r.InterludeExitLatchMut() == true);

    // Move to Ch2, then back into Interlude — second entry resets the
    // latch again (per cycle9 H3 once-per-visit contract).
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.Settle(w);
    w.Semester().Transition(SemesterState::Interlude_Market);
    r.Settle(w);
    CHECK(r.InterludeExitLatchMut() == false);
}
