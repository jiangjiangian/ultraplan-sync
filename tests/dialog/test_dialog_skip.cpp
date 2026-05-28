// Cycle 9.E (audit H2 / D5 / SC 2.2.2): pin the two skip-affordances
// SC 2.2.2 "Pause, Stop, Hide" demands for auto-updating content:
//
//   (a) HOLD-E to fast-advance dialog. The existing edge-IsPressed
//       (mash-E) path is preserved; HOLDING E for >= 300 ms then
//       auto-advances on a slow ~4-frame guard so a long-winded NPC
//       can be skimmed without finger pain.
//   (b) BACKSPACE to dismiss an on-screen HUD toast. Force-expires the
//       banner immediately, so a player who has read the line is not
//       forced to wait out the 4 s kHudTtl.
//
// The Backspace key was added to gfx::Key for this; the World gained a
// DismissHud() helper that snaps hudAge_ to kHudTtl (the same boundary
// HudExpired() / MessageView early-return already gate on, so the
// rendering contract is unchanged — the next View pass simply paints
// nothing). GameController consults the world LargeTargets() flag for
// the M2 site too; M2 is exercised by its own test (test_large_targets).
//
// Revert-verify (must FAIL without the H2 fix):
//   * Remove the hold-E auto-advance branch from src/GameController.cpp
//     → hold-E SUBCASE never advances (eHoldMs stays 0 effect).
//   * Remove the Backspace early-expire from src/GameController.cpp →
//     Backspace SUBCASE keeps hudAge < kHudTtl after the press.
//   * Remove World::DismissHud() → does not compile.

#include "doctest/doctest.h"
#include "world/World.h"
#include "controller/GameController.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"
#include "ui/MessageView.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include "engine/math/Vec2.h"

#include <set>
#include <string>
#include <vector>

using nccu::GameController;
using nccu::World;
using nccu::gfx::Key;

namespace {

// Mirrors the tests/test_menu_help.cpp TestInput pattern — a thin
// hand-driven InputSource so we can decouple key edges from a real
// device. Hold(k) sets IsDown true (and emits one IsPressed frame the
// first time it transitions); EndFrame() drops the per-frame pressed/
// released sets but leaves IsDown intact. Tap(k) holds then auto-
// releases on the next EndFrame for a single-frame press.
class TestInput final : public nccu::gfx::InputSource {
public:
    void Hold(Key k)    { if (down_.insert(static_cast<int>(k)).second) pressed_.insert(static_cast<int>(k)); }
    void Release(Key k) { if (down_.erase(static_cast<int>(k)))         released_.insert(static_cast<int>(k)); }
    void Tap(Key k)     { Hold(k); autoUp_.insert(static_cast<int>(k)); }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) { if (down_.erase(k)) released_.insert(k); }
        autoUp_.clear();
    }
    bool IsDown(Key k)     const noexcept override { return down_.count(static_cast<int>(k)) != 0; }
    bool IsPressed(Key k)  const noexcept override { return pressed_.count(static_cast<int>(k)) != 0; }
    bool IsReleased(Key k) const noexcept override { return released_.count(static_cast<int>(k)) != 0; }
private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

void Frame(GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

TEST_CASE("H2 hold-E fast-advances dialog past 300 ms; edge-E still works") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);    // 60 fps deterministic
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    // Open a multi-line dialog programmatically — the World owns the
    // DialogState directly, so we don't need to walk up to an NPC.
    // Six lines is enough to observe the auto-advance stepping past
    // multiple pages once the hold threshold fires.
    std::vector<std::string> lines{"L0", "L1", "L2", "L3", "L4", "L5"};
    world.Dialog().Open(lines);
    REQUIRE(world.Dialog().Active());
    CHECK(world.Dialog().CurrentLine() == "L0");

    SUBCASE("edge-E still steps once per tap (mash path preserved)") {
        in.Tap(Key::E);
        Frame(controller, in);                       // L0 -> L1
        CHECK(world.Dialog().Active());
        CHECK(world.Dialog().CurrentLine() == "L1");

        // A frame with no input does NOT advance — the held-E branch
        // is OFF (IsDown is false). This pins the "tap-only" semantics
        // for the prior behaviour: no eHoldMs leakage between presses.
        Frame(controller, in);
        CHECK(world.Dialog().CurrentLine() == "L1");

        in.Tap(Key::E);
        Frame(controller, in);                       // L1 -> L2
        CHECK(world.Dialog().CurrentLine() == "L2");
    }

    SUBCASE("hold-E for >= 300 ms then auto-advances on a frame guard") {
        // Frame 0: press E (edge fires) -> L0 advances to L1. The first
        // frame's press is the existing edge path; the hold timer starts
        // accumulating from this frame onward (IsDown is true).
        in.Hold(Key::E);
        Frame(controller, in);                       // edge: L0 -> L1
        CHECK(world.Dialog().CurrentLine() == "L1");

        // Hold for 10 more frames (~167 ms total) — well below the 300
        // ms threshold. No further advance happens; we're still on L1.
        // (10 was picked to stay comfortably below threshold even
        // accounting for float rounding on 1/60 s ticks.)
        for (int f = 0; f < 10; ++f) {
            Frame(controller, in);
        }
        CHECK(world.Dialog().CurrentLine() == "L1");

        // After another ~40 frames of hold (~833 ms total) the auto-
        // advance branch has fired several times on its ~4-frame
        // cooldown — at least once for sure, stepping past L1. The
        // test asserts the OBSERVABLE outcome: not still on L1.
        for (int f = 0; f < 40; ++f) {
            Frame(controller, in);
        }
        CHECK((world.Dialog().CurrentLine() != "L1"));

        in.Release(Key::E);
        Frame(controller, in);
    }

    SUBCASE("releasing E resets the hold timer; can't accumulate across presses") {
        // Hold for 10 frames (~167 ms), release, then hold again — the
        // timer must restart from 0, not pick up from where it left.
        in.Hold(Key::E);
        Frame(controller, in);                       // edge: L0 -> L1
        for (int f = 0; f < 9; ++f) {
            Frame(controller, in);
        }
        CHECK(world.Dialog().CurrentLine() == "L1");

        in.Release(Key::E);
        Frame(controller, in);                       // timer resets

        // Hold again for only 10 frames — total ACCUMULATED hold would
        // be 20+ frames if the timer leaked, but we expect zero auto-
        // advance because each press is < 300 ms in isolation.
        in.Hold(Key::E);
        Frame(controller, in);                       // edge: L1 -> L2
        for (int f = 0; f < 8; ++f) {
            Frame(controller, in);
        }
        // Still on L2 — the held timer never crossed 300 ms after the
        // release, so no auto-advance fired.
        CHECK(world.Dialog().CurrentLine() == "L2");
        in.Release(Key::E);
        Frame(controller, in);
    }

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("H2 Backspace force-expires the HUD toast (SC 2.2.2 skip-toast)") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    SUBCASE("Backspace on a fresh toast snaps HudAge to kHudTtl") {
        world.SetHudMessage("transient banner");
        REQUIRE_FALSE(world.HudMessage().empty());
        REQUIRE(world.HudAge() == doctest::Approx(0.0f));
        REQUIRE_FALSE(world.HudExpired());

        in.Tap(Key::Backspace);
        Frame(controller, in);

        // The Backspace path must have snapped the age to kHudTtl —
        // HudExpired() (its guard) is now true. MessageView::Draw and
        // the harness's HudExpired path both early-return at the same
        // boundary, so this dismiss is observable to View and to
        // state.jsonl alike.
        CHECK(world.HudExpired());
        CHECK(world.HudAge() >= nccu::kHudTtl);
    }

    SUBCASE("Backspace with no toast is a no-op (no spurious mutation)") {
        REQUIRE(world.HudMessage().empty());
        const float ageBefore = world.HudAge();

        in.Tap(Key::Backspace);
        Frame(controller, in);

        // hudMessage_ stays empty; the age was 0 (no message to tick),
        // and DismissHud's guard refuses to write into an empty toast.
        CHECK(world.HudMessage().empty());
        CHECK(world.HudAge() == doctest::Approx(ageBefore));
        CHECK_FALSE(world.HudExpired());
    }

    SUBCASE("Backspace is gated by Backspace alone — Enter/E don't dismiss") {
        world.SetHudMessage("transient banner");
        REQUIRE_FALSE(world.HudExpired());

        in.Tap(Key::E);
        Frame(controller, in);
        CHECK_FALSE(world.HudExpired());

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK_FALSE(world.HudExpired());

        // Now Backspace finally dismisses.
        in.Tap(Key::Backspace);
        Frame(controller, in);
        CHECK(world.HudExpired());
    }

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
