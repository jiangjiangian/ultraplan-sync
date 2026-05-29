#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/controller/GameController.h"
#include "game/quest/Chapter4Quest.h"
#include "game/state/EndingGate.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::gfx::Key;
using nccu::gfx::Vec2;

// G2 — the ending must NOT be abrupt: every Ch4 ending TRIGGER is deferred
// behind a brief inner-monologue (自白). These cases drive the REAL
// GameController::Update() loop (the same gfx::Input seam the harness +
// test_rain_survival use) and prove the deferral end-to-end: an ending does
// NOT fire while the 自白 dialog is on screen, and DOES fire once it closes.
// They also pin TryOpenEndingConfession's once-key idempotency and that the
// 自白 matches the ending the gate will land on.

namespace {

// Never presses a key — the player stands still; only the per-frame
// rain/confession/gate logic runs. Mirrors test_rain_survival's TestInput.
class TestInput final : public nccu::gfx::InputSource {
public:
    void EndFrame() {}
    bool IsDown(Key)     const noexcept override { return false; }
    bool IsPressed(Key)  const noexcept override { return false; }
    bool IsReleased(Key) const noexcept override { return false; }
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

// Build a World already in Ch4 with the GameController wired, give the
// player an umbrella so the rain tick DRAINS (never the lethal teleport
// that would perturb a multi-frame deferral test), and return refs.
struct Ch4Fixture {
    World world;
    nccu::GameController controller;
    TestInput in;
    explicit Ch4Fixture()
        : world("", /*loadSprites=*/false), controller(world, EventBus::Instance()) {
        nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
        nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
        EventBus::Instance().Clear();
        world.Semester().Transition(SemesterState::Chapter4_Finals);
        nccu::gfx::Input::SetSource(&in);
        // Settle the Ch4 entry side-effects (clears HasUmbrella +
        // Flag_HasTrueUmbrella) with one quiet frame, THEN arm the player.
        Frame(controller, in);
    }
    ~Ch4Fixture() {
        nccu::gfx::Input::SetSource(nullptr);
        nccu::gfx::Time::SetFixedStep(0.0f);
        EventBus::Instance().Clear();
    }
    Player& P() { return *world.GetPlayer(); }
};

}  // namespace

TEST_CASE("G2: buy-ugly ending DEFERS behind the 自白, then resolves to C on close") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);                       // keep rain draining, not lethal
    REQUIRE(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);
    REQUIRE_FALSE(fx.world.Dialog().Active());

    // Arm the Ending-C trigger (what Vendor::TryBuy sets on the 集英樓 醜傘).
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    // Next frame: the non-dialog poll opens the 務實 自白 and the ending
    // gate DEFERS behind it — the player must read the monologue first.
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Dialog().Active());                                  // 自白 up
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);  // NOT yet

    // Even running more frames while the box is up changes nothing (the
    // dialog freeze early-returns; the ending stays deferred).
    for (int i = 0; i < 10; ++i) Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);

    // Player finishes reading: close the box. The next non-dialog poll
    // resolves the (persistent) flag to Ending C.
    fx.world.Dialog().Close();
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_C);
}

TEST_CASE("G2: cursed ending DEFERS behind the 自白, then resolves to B on close") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);
    p.SetFlag(nccu::kFlagTookCursedUmbrella);         // carried from Ch1 -> Ending B

    Frame(fx.controller, fx.in);
    CHECK(fx.world.Dialog().Active());                                  // curse 自白
    CHECK(fx.world.Semester().Current() == SemesterState::Chapter4_Finals);

    fx.world.Dialog().Close();
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_B);
}

TEST_CASE("G2: the 自白 is one-shot (once-key) — it never re-opens after reading") {
    Ch4Fixture fx;
    Player& p = fx.P();
    p.SetHasUmbrella(true);
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    Frame(fx.controller, fx.in);
    REQUIRE(fx.world.Dialog().Active());
    CHECK(p.HasFlag(nccu::kFlagCh4ConfessedUgly));   // once-key set
    fx.world.Dialog().Close();

    // Resolve to C; the confession must not fire a second time on the way.
    Frame(fx.controller, fx.in);
    CHECK(fx.world.Semester().Current() == SemesterState::Ending_C);
}

// Direct unit on the helper: it opens the matching confession exactly once,
// respects the gate precedence (cursed outranks ugly/true), and no-ops while
// a dialog is already active / outside Ch4. Revert-verify: delete the
// dialog.Open() calls and the Active() CHECKs fail.
TEST_CASE("G2 unit: TryOpenEndingConfession picks one confession, once, by precedence") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    EventBus::Instance().Clear();

    SUBCASE("outside Ch4 -> no-op") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter1_AddDrop));
        CHECK_FALSE(d.Active());
    }
    SUBCASE("cursed outranks ugly (matches the B-over-C gate)") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        CHECK(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));
        CHECK(d.Active());
        CHECK(p.HasFlag(nccu::kFlagCh4ConfessedCursed));   // cursed chosen
        CHECK_FALSE(p.HasFlag(nccu::kFlagCh4ConfessedUgly));
    }
    SUBCASE("never interrupts an open dialog; once-key blocks a re-open") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        d.Open({"some other conversation"});
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // box up -> no-op
        d.Close();
        CHECK(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // now opens
        d.Close();
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // once-key -> no-op
    }
    SUBCASE("reclaimed-true 自白 only BEFORE the finale (no double beat)") {
        Player p{Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);              // gentle finale path
        CHECK_FALSE(nccu::TryOpenEndingConfession(
            p, d, SemesterState::Chapter4_Finals));        // suppressed
        Player q{Vec2{0, 0}}; nccu::DialogState d2;
        q.SetFlag(nccu::kFlagHasTrueUmbrella);                 // ground reclaim, no finale
        CHECK(nccu::TryOpenEndingConfession(
            q, d2, SemesterState::Chapter4_Finals));
        CHECK(q.HasFlag(nccu::kFlagCh4ConfessedTrue));
    }
    EventBus::Instance().Clear();
}
