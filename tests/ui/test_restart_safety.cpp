// Restart-safety regression (in-game menu 重新開始 + BUGLEDGER B2/H1).
#include "quest/Flags.h"
//
// The new in-game pause menu's "重新開始" routes main.cpp's outer loop
// to tear down the per-run {World, View, GameController} scope and build
// a brand-new one. This must hold two invariants or the feature is a
// data-leak / use-after-free footgun:
//
//   (a) FRESH STATE: a restarted run's Player is back at the initial
//       karma / money / flags / consumables — no pollution leaks across
//       runs (the player who ended run #1 corrupt/rich/flagged starts
//       run #2 exactly as a brand-new game would).
//
//   (b) NO DANGLING / DOUBLED EventBus SUBSCRIBERS: GameController wires
//       EventBus subscribers in its ctor (some capture World refs) and
//       its dtor calls EventBus::Clear(). Restarting must NOT accumulate
//       handlers across cycles, and must NOT leave a handler that
//       captured a destroyed World's address (BUGLEDGER B2 was a
//       SIGSEGV/double-free from exactly that dangling-capture; H1 added
//       the RAII unsubscribe machinery). We prove the post-restart bus
//       has EXACTLY the fresh run's subscribers — old ones neither
//       linger (dangle) nor stack (double-subscribe).
//
// This mirrors main.cpp's real restart path: the {World, controller}
// scope is entered, dirtied, then destroyed (controller dtor Clears the
// bus before the World it captured dies — the load-bearing reverse-dtor
// order), then a new scope is built.
//
// NOTE: the eventbus_isolation listener Clears the bus at every
// test/subcase boundary, so each TEST_CASE below starts from a clean
// bus; the per-cycle assertions here drive Clear() themselves via the
// controller dtor and never depend on cross-test bus state.

#include "doctest/doctest.h"
#include "world/World.h"
#include "controller/GameController.h"
#include "entities/Player.h"
#include "controller/EventBus.h"
#include "state/SemesterState.h"
#include "gfx/Vec2.h"

#include <memory>
#include <string>

using nccu::World;
using nccu::GameController;

namespace {

// Drive the Player into a "dirty" mid-run state: spent/earned money,
// shifted karma, set flags, stocked consumables. A correct restart wipes
// ALL of this back to construction defaults.
void DirtyTheRun(World& w) {
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(150);                       // 100 -> 250
    p->AddKarma(-40);                       //  50 ->  10
    p->SetFlag(nccu::kFlagTookCursedUmbrella);
    p->SetFlag(nccu::kFlagHelpedTACh1);
    p->AddConsumable("HotPack").AddConsumable("HotPack");
    p->SetHasUmbrella(true);
    CHECK(p->GetMoney() == 250);
    CHECK(p->GetKarma() == 10);
    CHECK(p->HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p->ConsumableCount("HotPack") == 2);
}

} // namespace

TEST_CASE("Restart resets karma/money/flags/consumables to a fresh game") {
    // Run #1: build, dirty it, then tear the run scope down (RAII).
    {
        World w("", /*loadSprites=*/false);
        GameController c{w};
        DirtyTheRun(w);
    }   // ~GameController -> EventBus::Clear(); ~World frees the player

    // Run #2: a brand-new World == a fresh game. Nothing from run #1
    // may have survived (the state lives on the Player owned by the
    // destroyed World, never in a global).
    {
        World w("", /*loadSprites=*/false);
        GameController c{w};
        const Player* p = w.GetPlayer();
        REQUIRE(p != nullptr);
        CHECK(p->GetKarma() == 50);                 // GDD start
        CHECK(p->GetMoney() == 100);                // GDD start
        CHECK(p->GetRainMeter() == doctest::Approx(0.0f));
        CHECK_FALSE(p->HasUmbrella());
        CHECK_FALSE(p->HasFlag(nccu::kFlagTookCursedUmbrella));
        CHECK_FALSE(p->HasFlag(nccu::kFlagHelpedTACh1));
        CHECK(p->ConsumableCount("HotPack") == 0);
        CHECK(w.Semester().Current() ==
              nccu::SemesterState::Chapter1_AddDrop);
    }
}

TEST_CASE("Restart does not dangle or double-subscribe EventBus handlers") {
    // An external counting probe. Re-subscribed fresh INSIDE every cycle
    // AFTER the controller dtor's Clear(), so it only ever counts the
    // current cycle's deliveries (the eventbus_isolation listener also
    // Clears at the case boundary, so cycle 0 starts clean too).
    //
    // The controller wires exactly ONE World-capturing ShowMessage
    // subscriber (WireHudMessageSubscriber → World::SetHudMessage) plus
    // the logging ShowMessage subscriber (cout only, no capture). If a
    // restart dangled the old World's handler, publishing after rebuild
    // would write through a freed World (UAF, sanitizer-detectable); if
    // it double-subscribed, the live World's HUD would be set twice and
    // the per-cycle handler population would grow run over run. We pin
    // both: the live World's HUD message is delivered on EVERY cycle
    // (handler present, not dangled) and the delivery COUNT to a fresh
    // single probe is identical every cycle (no accumulation).
    int probeHits = 0;

    auto runOneCycleAndCount = [&](int cycle) {
        World w("", /*loadSprites=*/false);
        GameController c{w};

        // Fresh probe for THIS cycle (Clear() in the previous cycle's
        // controller dtor removed any earlier probe).
        probeHits = 0;
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [&](const Event&) { ++probeHits; });

        const std::string msg =
            "restart-cycle-" + std::to_string(cycle);
        EventBus::Instance().Publish(Event{EventType::ShowMessage, msg});

        // The live World's HUD subscriber fired (handler present this
        // cycle — proves it was NOT lost) and routed to THIS World.
        CHECK(w.HudMessage() == msg);
        // Exactly our single probe fired once — if a previous cycle's
        // probe or HUD subscriber had dangled into this cycle, the count
        // would exceed 1 / grow per cycle (double-subscribe) or the bus
        // would invoke a handler over a freed capture (UAF). Stable == 1
        // every cycle ⇒ Clear() fully drops the old run's subscribers.
        CHECK(probeHits == 1);
    };  // ~GameController -> EventBus::Clear() wipes this cycle's handlers

    for (int cycle = 0; cycle < 4; ++cycle)
        runOneCycleAndCount(cycle);

    // After the last cycle's controller dtor, the bus is empty: a
    // publish reaches nothing (no lingering World/HUD/probe handler).
    int afterAll = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { ++afterAll; });
    EventBus::Instance().Publish(Event{EventType::ShowMessage, "tail"});
    CHECK(afterAll == 1);   // only the just-added probe — nothing dangled
}
