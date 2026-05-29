// Cycle 9.B H5 — KarmaChanged is no longer a dead channel.
//
// Before this fix, Player::AddKarma silently mutated karma_ and only
// CursedUmbrella ever Publish()ed KarmaChanged (the diagnostic playtest
// found 1 publisher / 0 subscribers — pure dead-letter). These tests
// pin three guarantees:
//   1. Every AddKarma call now publishes KarmaChanged with the signed
//      delta as text ("+5", "-3").
//   2. The WireKarmaToastSubscriber consumer turns that into a
//      ShowMessage prefixed with 業力, which is what the HUD banner
//      eventually mirrors.
//   3. CursedUmbrella::beClaimed (which goes through decreaseKarma →
//      AddKarma) no longer double-publishes KarmaChanged.
//
// The eventbus_isolation listener clears the bus between cases, so each
// test starts from a clean slate even though all of them touch the
// global EventBus singleton.

#include "doctest/doctest.h"
#include "game/entities/CursedUmbrella.h"
#include "engine/events/EventBus.h"
#include "game/controller/EventWiring.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "engine/math/Vec2.h"

#include <string>
#include <vector>

using nccu::engine::math::Vec2;

namespace {

// Capture every KarmaChanged payload published during the test body.
struct KarmaCapture {
    std::vector<std::string> deltas;
    EventBus::Subscription   sub;
};

KarmaCapture CaptureKarma() {
    KarmaCapture cap;
    cap.sub = EventBus::Instance().ScopedSubscribe(
        EventType::KarmaChanged,
        [&cap](const Event& e) { cap.deltas.push_back(e.text); });
    return cap;
}

} // namespace

TEST_CASE("AddKarma publishes KarmaChanged with signed delta text") {
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    p.AddKarma(5);
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "+5");

    p.AddKarma(-3);
    REQUIRE(cap.deltas.size() == 2);
    CHECK(cap.deltas[1] == "-3");

    // The signed format is %+d so positive deltas always carry an
    // explicit '+' — the karma subscriber relies on this to render
    // "業力 +5" vs "業力 -3" without a separate branch.
    CHECK(p.GetKarma() == 50 + 5 - 3);
}

TEST_CASE("decreaseKarma forwards through AddKarma — single publish only") {
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    p.decreaseKarma(10);
    // ONE event from one call: AddKarma's publish, not a duplicate
    // from a separate decreaseKarma site. Pins the "no double-publish"
    // invariant that the Cycle 9.B fix to CursedUmbrella depends on.
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "-10");
}

TEST_CASE("P2: CursedUmbrella::beClaimed publishes NO KarmaChanged (pickup is karma-neutral)") {
    // Pre-P2 the cursed pickup ran .decreaseKarma(30) inline, which fired
    // ONE KarmaChanged "-30" via Player::AddKarma's auto-publish (Cycle
    // 9.B's de-duplication win). P2 moves the karma cost off the pickup
    // and onto per-chapter ApplyCursedTaintDecay (SceneRouter Ch2/3/4
    // entry), so the pickup itself emits ZERO KarmaChanged — the
    // visible "業力 -5" banner now lands at the next chapter boundary
    // instead, where the moral stain is actually compounding.
    Player p{Vec2{0, 0}};
    auto cap = CaptureKarma();

    CursedUmbrella umb{Vec2{0, 0}};
    umb.beClaimed(&p);

    CHECK(cap.deltas.empty());                       // no KarmaChanged at pickup
    CHECK(p.GetKarma() == 50);                       // karma untouched
    CHECK(p.GetCursedTaint() == 1);                  // taint bumped instead

    // Now drive the per-chapter decay: ONE KarmaChanged "-5" fires.
    p.ApplyCursedTaintDecay();
    REQUIRE(cap.deltas.size() == 1);
    CHECK(cap.deltas[0] == "-5");
    CHECK(p.GetKarma() == 45);
}

TEST_CASE("WireKarmaToastSubscriber turns KarmaChanged into HUD toast") {
    // End-to-end: AddKarma -> KarmaChanged -> WireKarmaToastSubscriber
    // -> ShowMessage -> WireHudMessageSubscriber -> World.HudMessage().
    // This is the full production wiring exercised inside the unit
    // test process (no GameController construction needed).
    //
    // NOTE: the EventBus isolation reporter (test_eventbus_isolation)
    // clears the bus at every subcase boundary, so the Wire... calls
    // MUST live INSIDE each SUBCASE — a subscription at TEST_CASE scope
    // is wiped before the body runs (same idiom as the
    // "ChapterGate Interlude -> returnTo" case in test_chapter_transitions).

    SUBCASE("positive delta -> 業力 +N") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        p.AddKarma(5);
        // The HUD message must contain BOTH the 業力 prefix and the
        // signed delta — the brief's expected payload from H5.
        CHECK(w.HudMessage().find("業力") != std::string::npos);
        CHECK(w.HudMessage().find("+5") != std::string::npos);
    }

    SUBCASE("negative delta -> 業力 -N") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        p.AddKarma(-3);
        CHECK(w.HudMessage().find("業力") != std::string::npos);
        // The actual rendering carries the sign verbatim so a player
        // can read 業力 -3 without ambiguity.
        CHECK(w.HudMessage().find("-3") != std::string::npos);
    }

    SUBCASE("AddKarma(0) emits no HUD toast") {
        nccu::World w{"", /*loadSprites=*/false};
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);
        nccu::WireKarmaToastSubscriber(EventBus::Instance());
        Player p{Vec2{0, 0}};

        // 0-delta calls happen rarely (defensive call sites). The
        // subscriber filters "+0" / "-0" so they never burn the HUD
        // banner with a dummy reading. World starts with empty HUD.
        REQUIRE(w.HudMessage().empty());
        p.AddKarma(0);
        CHECK(w.HudMessage().empty());
    }
}
