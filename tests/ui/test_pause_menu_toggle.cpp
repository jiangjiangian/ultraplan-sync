// Cycle 9.E.3 (audit M2 / M3): pin the pause-menu UI for the
// ReducedMotion and LargeTargets accessibility flags. 9.E.1/9.E.2
// added the World flags + env-var hooks but the player still had no
// non-env-var way to flip them; this cycle wires Enter on the two new
// pause-menu toggle rows into World::SetReducedMotion /
// SetLargeTargets, mirroring the existing 4-row menu's input loop.
//
// Pinned via GameController's REAL input loop (not direct
// world.SetMenuCursor calls) so the cursor-range bump from 4 → 6
// rows + the toggle-row switch arms are both exercised end-to-end:
//
//   (a) kMenuItemCount is 6 and ↓ ↑ walks the full 0..5 range with
//       modular wraparound (no off-by-one at the new boundary).
//   (b) Enter on row 2 flips World::ReducedMotion() (with the menu
//       staying open and the cursor staying on row 2 so the [開]/[關]
//       label change is visible to the player).
//   (c) Enter on row 3 flips World::LargeTargets() with the same
//       in-place semantics; the two flags are independent (toggling
//       one does not perturb the other — same orthogonality the
//       test_large_targets sibling pins on the raw setters).
//   (d) Both toggles round-trip: a second Enter on the same row
//       restores the previous state (so the cursor never deadlocks).
//
// Revert-verify (must FAIL without 9.E.3):
//   * Drop the new case-2 / case-3 arms in GameController → (b)/(c)
//     fail: the flag never flips even though Enter fires.
//   * Restore kMenuItemCount = 4 → (a) fails: walking ↓ five times
//     wraps at index 3, not 5.
//   * Swap the two case bodies → (b)/(c) cross-pollute and the
//     orthogonality check trips.

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "controller/GameController.h"
#include "entities/Player.h"
#include "world/World.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <cstdlib>
#include <set>

using nccu::GameController;
using nccu::World;
using nccu::gfx::Key;

namespace {

// Same TestInput shape as test_menu_help.cpp — a minimal InputSource
// the GameController consults via the Input static facade. Tap()
// queues an edge-triggered IsPressed for ONE frame; EndFrame() must
// be called after each c.Update() so the auto-release runs.
class TestInput final : public nccu::gfx::InputSource {
public:
    void Hold(Key k) {
        if (down_.insert(static_cast<int>(k)).second)
            pressed_.insert(static_cast<int>(k));
    }
    void Release(Key k) {
        if (down_.erase(static_cast<int>(k)))
            released_.insert(static_cast<int>(k));
    }
    void Tap(Key k) {
        Hold(k);
        autoUp_.insert(static_cast<int>(k));
    }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) {
            if (down_.erase(k)) released_.insert(k);
        }
        autoUp_.clear();
    }
    bool IsDown(Key k) const noexcept override {
        return down_.count(static_cast<int>(k)) != 0;
    }
    bool IsPressed(Key k) const noexcept override {
        return pressed_.count(static_cast<int>(k)) != 0;
    }
    bool IsReleased(Key k) const noexcept override {
        return released_.count(static_cast<int>(k)) != 0;
    }
private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

void Frame(GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

TEST_CASE("9.E.3 (a) pause menu now has 6 rows, cursor wraps 0..5") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);                         // settle
    CHECK(World::kMenuItemCount == 6);

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());
    CHECK(world.MenuCursor() == 0);

    // Walk ↓ five times — the cursor must hit every row index 1..5.
    for (int expected = 1; expected <= 5; ++expected) {
        in.Tap(Key::Down);
        Frame(controller, in);
        CHECK(world.MenuCursor() == expected);
    }

    // One more ↓ wraps to 0 (modular MoveMenuCursor).
    in.Tap(Key::Down);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 0);

    // ↑ from 0 wraps to 5 (the other direction of the same modulus).
    in.Tap(Key::Up);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 5);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("9.E.3 (b) Enter on row 2 toggles World.ReducedMotion()") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);
    REQUIRE_FALSE(world.ReducedMotion());

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // Walk to row 2 (減少動畫).
    in.Tap(Key::Down);
    Frame(controller, in);
    in.Tap(Key::Down);
    Frame(controller, in);
    REQUIRE(world.MenuCursor() == 2);

    // Enter flips ReducedMotion on; menu stays open; cursor unchanged.
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.ReducedMotion());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 2);
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // Second Enter flips it back off — round-trip, no latch.
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK_FALSE(world.ReducedMotion());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 2);

    // LargeTargets must stay default throughout — toggles are orthogonal.
    CHECK_FALSE(world.LargeTargets());

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("9.E.3 (c) Enter on row 3 toggles World.LargeTargets()") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);
    REQUIRE_FALSE(world.LargeTargets());

    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // Walk to row 3 (擴大目標).
    for (int i = 0; i < 3; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    REQUIRE(world.MenuCursor() == 3);

    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.LargeTargets());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 3);
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // Second Enter flips back off.
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK_FALSE(world.LargeTargets());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 3);

    // ReducedMotion must stay default throughout — toggles orthogonal.
    CHECK_FALSE(world.ReducedMotion());

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("9.E.3 (d) destructive rows still map correctly after the shift") {
    // The new 6-row order shifts 重新開始 from index 2 → 4 and 離開
    // from 3 → 5. Pin both: Restart on row 4 must request
    // AppAction::Restart (not None / not Quit); Quit on row 5 must
    // request AppAction::Quit. Pure intent — main.cpp's outer loop
    // is what acts on it; here we just verify the controller routes
    // the right enum from the right index.
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    SUBCASE("row 4 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::gfx::Input::SetSource(&in);
        Frame(controller, in);

        in.Tap(Key::M);
        Frame(controller, in);
        for (int i = 0; i < 4; ++i) {
            in.Tap(Key::Down);
            Frame(controller, in);
        }
        REQUIRE(world.MenuCursor() == 4);

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);

        // Toggles untouched.
        CHECK_FALSE(world.ReducedMotion());
        CHECK_FALSE(world.LargeTargets());

        nccu::gfx::Input::SetSource(nullptr);
    }

    SUBCASE("row 5 → Quit") {
        World world("", /*loadSprites=*/false);
        GameController controller{world, EventBus::Instance()};
        TestInput in;
        nccu::gfx::Input::SetSource(&in);
        Frame(controller, in);

        in.Tap(Key::M);
        Frame(controller, in);
        for (int i = 0; i < 5; ++i) {
            in.Tap(Key::Down);
            Frame(controller, in);
        }
        REQUIRE(world.MenuCursor() == 5);

        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Quit);

        nccu::gfx::Input::SetSource(nullptr);
    }

    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("menu opens on M, never on ESC (ESC is the program quit key)") {
    // The pause menu used to toggle on ESC. But ESC is raylib's default
    // exit key, so main.cpp's WindowShouldClose loops quit on it — binding
    // the menu to ESC also flashed the menu for one frame before the
    // window closed. The menu now lives on M; GameController never reads
    // ESC, so a frozen Update treats it as a no-op and ESC's only role
    // (in the real app) is the clean program exit owned by main.cpp.
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                         // settle

    // ESC must NOT open the menu.
    in.Tap(Key::Escape);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());

    // M opens it.
    in.Tap(Key::M);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());

    // ESC must NOT close it either (no resume-on-ESC).
    in.Tap(Key::Escape);
    Frame(controller, in);
    CHECK(world.MenuOpen());

    // M toggles it back closed.
    in.Tap(Key::M);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
