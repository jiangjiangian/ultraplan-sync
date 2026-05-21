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

// Cycle 10.P0a (awsome_cpp.md §6) + 10.P0b (L8 fix): SceneRouter owns
// the chapter/interlude/ending transition observer that used to live
// inline on GameController. The fix splits the observation into two
// halves so the visible bug (npcs[] list lagging by 1 frame) closes
// without touching the harness state.jsonl observable timeline:
//
//   SettleRoster        — end-of-Update; roster swap ONLY.
//   SettleSideEffects   — top-of-Update; player pos / consumables / events.
//
// These tests pin the contract:
//   1. SettleRoster on a state change actually respawns the NPCs.
//   2. SettleRoster is idempotent on its own cursor.
//   3. SettleSideEffects on Interlude entry moves the player, wipes
//      consumables, publishes the arrival hint, resets the exit latch.
//   4. SettleSideEffects on Ch4 entry clears the umbrella state.
//   5. Calling SettleRoster THEN SettleSideEffects in the same tick
//      keeps the side-effect half byte-equivalent to the pre-Cycle 10
//      single-block behaviour — no double-spawn, no double-publish.

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

TEST_CASE("SceneRouter ctor: cursors start at the initial state") {
    SceneRouter r{SemesterState::Chapter1_AddDrop};
    CHECK(r.LastRosterState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
    CHECK(r.InterludeExitLatchMut() == false);
}

TEST_CASE("SettleRoster: no-op when the FSM hasn't moved") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 roster present

    r.SettleRoster(w);                     // FSM still Ch1
    CHECK(HasNpcId(w, "victim"));          // unchanged
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter1_AddDrop);
}

TEST_CASE("SettleRoster: respawns the new chapter's NPCs on a transition") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPC

    // FSM advances WITHOUT going through the SceneRouter (mimics a
    // CheckChapterGates / EventWiring transition firing during the
    // controller's Update body).
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // Ch2 roster now visible to the View (and state.jsonl npcs[]) on
    // the SAME frame the FSM moved. Pre-fix this was 1 frame late.
    CHECK(HasNpcId(w, "librarian"));       // Ch2-only quest-giver
    CHECK(HasNpcId(w, "victim"));          // victim is Ch2 too
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
}

TEST_CASE("SettleRoster: SettleSideEffects cursor untouched (split is real)") {
    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleRoster(w);

    // The "side-effect cursor" must NOT have advanced — it tracks the
    // top-of-Update half, which the test hasn't invoked yet. If both
    // cursors moved together, the next SettleSideEffects call would
    // be a no-op and the player would never be repositioned.
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState()        == SemesterState::Chapter1_AddDrop);
}

TEST_CASE("SettleSideEffects Interlude entry: pos, consumables, hint, latch") {
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
    r.SettleSideEffects(w);

    // All four observable side effects fire together:
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // wiped
    CHECK(latestHud == nccu::kInterludeArrivalHint);
    CHECK(r.InterludeExitLatchMut() == false);

    // Cursor stamped so a re-call is a no-op.
    CHECK(r.LastRosterState() == SemesterState::Interlude_Market);
    p->SetPosition(nccu::gfx::Vec2{42.0f, 42.0f});
    r.SettleSideEffects(w);                            // idempotent
    CHECK(p->GetPosition().x == doctest::Approx(42.0f));

    EventBus::Instance().Clear();
}

TEST_CASE("SettleSideEffects Ch4 entry: umbrella + TrueUmbrella flag reset") {
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};

    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->SetHasUmbrella(true);
    p->SetFlag("Flag_HasTrueUmbrella");

    // FSM jumps straight to Ch4 (mimics ChapterGate Interlude→Ch4).
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    r.SettleSideEffects(w);

    // chapter4.md L6「傘再度失蹤」: the player walks out of 集英樓 with
    // no umbrella. The fix's Ch4 branch is the GDD enforcement.
    CHECK_FALSE(p->HasUmbrella());
    CHECK_FALSE(p->HasFlag("Flag_HasTrueUmbrella"));
    CHECK(r.LastRosterState() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

TEST_CASE("L8 fix end-to-end: SettleRoster THEN SettleSideEffects on a tick") {
    // Pin the full Cycle 10.P0b flow: a transition fires mid-frame
    // (after dialog branch / E-probe / CheckChapterGates / etc.); the
    // controller calls SettleRoster at end-of-Update so the View's
    // upcoming Draw paints with the new chapter's NPCs. On the NEXT
    // frame's top-of-Update, SettleSideEffects fires the player-pos
    // teleport + arrival hint. Both halves run exactly once per
    // transition (idempotent under their own cursors).
    EventBus::Instance().Clear();
    std::string latestHud;
    auto sub = SubscribeToLatest(latestHud);

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);

    // Simulate the umbrella-claim transition without driving the full
    // event bus: just move the FSM (the SceneRouter cares about the
    // FSM state only — it doesn't poll the EventBus).
    w.Semester().Transition(SemesterState::Interlude_Market);

    // ---- Tick N: end-of-Update SettleRoster ----
    REQUIRE(HasNpcId(w, "victim"));        // Ch1 NPCs still present pre-call
    r.SettleRoster(w);
    CHECK_FALSE(HasNpcId(w, "victim"));    // IL roster is empty
    // No player teleport yet — the harness observable is unchanged.
    // (The test doesn't move the player itself; we just verify the
    // SettleRoster pass did NOT publish the arrival hint or wipe
    // consumables — that's SettleSideEffects' job on the NEXT tick.)
    CHECK(latestHud.empty());

    // ---- Tick N+1: top-of-Update SettleSideEffects ----
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(nccu::kInterludeEntry.x));
    CHECK(p->GetPosition().y == doctest::Approx(nccu::kInterludeEntry.y));
    CHECK(latestHud == nccu::kInterludeArrivalHint);

    // Both halves idempotent on a repeat call.
    p->SetPosition(nccu::gfx::Vec2{7.0f, 8.0f});
    latestHud.clear();
    r.SettleRoster(w);
    r.SettleSideEffects(w);
    CHECK(p->GetPosition().x == doctest::Approx(7.0f));  // not re-teleported
    CHECK(latestHud.empty());                            // no re-publish

    EventBus::Instance().Clear();
}

TEST_CASE("SettleSideEffects defensively respawns the roster if SettleRoster was skipped") {
    // The Cycle 10 split keeps both halves runnable individually so
    // callers that bypass SettleRoster (e.g. tests, or a future code
    // path that only goes through the top-of-Update branch) still get
    // a coherent roster. SettleSideEffects calls RespawnChapterRoster
    // when its respawn cursor disagrees with the FSM state.
    EventBus::Instance().Clear();

    World w("", /*loadSprites=*/false);
    SceneRouter r{w.Semester().Current()};
    REQUIRE(HasNpcId(w, "victim"));        // Ch1

    // FSM moves without going through SettleRoster first.
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    r.SettleSideEffects(w);

    // Roster is now Ch2 — the defensive RespawnChapterRoster fired.
    CHECK(HasNpcId(w, "librarian"));       // Ch2 NPC
    CHECK(r.LastRosterRespawnState() == SemesterState::Chapter2_Midterms);
    CHECK(r.LastRosterState() == SemesterState::Chapter2_Midterms);

    EventBus::Instance().Clear();
}
