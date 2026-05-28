// A-T3 — the ending screen's bottom 3-option menu (回首頁 / 重新開始 / 結束).
//
// The ending screen used to be a passive card with no agency; the owner
// asked for "結局加三個左右選項" — a ←/→ selectable, E/Enter confirmable menu
// that routes back to the title, restarts a fresh game, or truly quits.
// This pins TWO layers:
//
//   (1) The PURE index → choice map (EndingMenuChoiceAt), independent of any
//       World / renderer — so the menu's meaning is testable in isolation.
//
//   (2) The LIVE GameController wiring: while the semester is an ending
//       state the world is FROZEN and the ending-menu keys are consumed —
//       ←/→ move World::EndingMenuCursor (modular), E/Enter map the cursor
//       to World::PendingAppAction (回首頁/重新開始 → Restart, 結束 → Quit).
//       Driven through the controller's REAL input loop (not direct setter
//       calls), exactly like test_pause_menu_toggle's (d) destructive-row
//       test, so the cursor range + the cursor→AppAction switch are
//       exercised end-to-end.
//
// Revert-verify (must FAIL without A-T3):
//   * Drop the top-of-Update IsEndingState block in GameController → the
//     cursor never moves and E/Enter never sets an AppAction (the world
//     would instead try to simulate on the ending screen).
//   * Map 結束 to Restart (or 回首頁 to Quit) → the cursor→AppAction
//     assertions trip.
//   * Make MoveEndingMenuCursor non-modular → the wrap checks trip.

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "controller/GameController.h"
#include "entities/Player.h"
#include "world/World.h"
#include "ui/EndingView.h"
#include "state/SemesterState.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"

#include <cstdlib>
#include <set>

using nccu::GameController;
using nccu::World;
using nccu::EndingMenuChoice;
using nccu::gfx::Key;

namespace {

// Same minimal InputSource shape as test_pause_menu_toggle / test_menu_help.
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
    void Tap(Key k) { Hold(k); autoUp_.insert(static_cast<int>(k)); }
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) if (down_.erase(k)) released_.insert(k);
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

void Frame(GameController& c, TestInput& in) { c.Update(); in.EndFrame(); }

// Put the World into a given ending state (the gate logic is owned
// elsewhere; the menu only cares THAT we are in an ending).
void EnterEnding(World& w, nccu::SemesterState s) {
    w.Semester().Transition(s);
    REQUIRE(nccu::IsEndingState(w.Semester().Current()));
}

}  // namespace

// ---- (1) the pure index → choice map -----------------------------------
TEST_CASE("A-T3: EndingMenuChoiceAt maps 0/1/2 to Title/Restart/Quit") {
    CHECK(nccu::EndingMenuChoiceAt(0) == EndingMenuChoice::BackToTitle);
    CHECK(nccu::EndingMenuChoiceAt(1) == EndingMenuChoice::RestartGame);
    CHECK(nccu::EndingMenuChoiceAt(2) == EndingMenuChoice::Quit);
    // Out-of-range clamps into the valid set (never "nothing").
    CHECK(nccu::EndingMenuChoiceAt(3)  == EndingMenuChoice::BackToTitle);
    CHECK(nccu::EndingMenuChoiceAt(-1) == EndingMenuChoice::Quit);
    CHECK(nccu::World::kEndingMenuItemCount == 3);
    // Each choice has a non-empty label (the renderer / glyph-scan rely on it).
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::BackToTitle).empty());
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::RestartGame).empty());
    CHECK_FALSE(nccu::EndingMenuLabel(EndingMenuChoice::Quit).empty());
}

// ---- (2) live controller wiring ----------------------------------------
TEST_CASE("A-T3: on the ending screen ←/→ move the cursor (modular)") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    EnterEnding(world, nccu::SemesterState::Ending_D);
    Frame(controller, in);                 // settle on the ending screen
    CHECK(world.EndingMenuCursor() == 0);  // starts on 回首頁

    in.Tap(Key::Right);
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 1);  // 重新開始

    in.Tap(Key::Right);
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 2);  // 結束

    in.Tap(Key::Right);                    // wraps 2 -> 0
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 0);

    in.Tap(Key::Left);                     // wraps 0 -> 2 (other direction)
    Frame(controller, in);
    CHECK(world.EndingMenuCursor() == 2);

    // No confirm pressed yet ⇒ no app action requested.
    CHECK(world.PendingAppAction() == World::AppAction::None);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("A-T3: ending-menu confirm maps the cursor to the right AppAction") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    // 回首頁 (cursor 0) → Restart (back to title).
    SUBCASE("回首頁 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world};
        TestInput in;
        nccu::gfx::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_A);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 0);
        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);
        nccu::gfx::Input::SetSource(nullptr);
    }

    // 重新開始 (cursor 1) → Restart (fresh game via the title).
    SUBCASE("重新開始 → Restart") {
        World world("", /*loadSprites=*/false);
        GameController controller{world};
        TestInput in;
        nccu::gfx::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_B);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 1);
        in.Tap(Key::E);                    // E confirms too (not just Enter)
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Restart);
        nccu::gfx::Input::SetSource(nullptr);
    }

    // 結束 (cursor 2) → Quit (the ONLY path that closes the window).
    SUBCASE("結束 → Quit") {
        World world("", /*loadSprites=*/false);
        GameController controller{world};
        TestInput in;
        nccu::gfx::Input::SetSource(&in);
        EnterEnding(world, nccu::SemesterState::Ending_C);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        in.Tap(Key::Right);
        Frame(controller, in);
        REQUIRE(world.EndingMenuCursor() == 2);
        in.Tap(Key::Enter);
        Frame(controller, in);
        CHECK(world.PendingAppAction() == World::AppAction::Quit);
        nccu::gfx::Input::SetSource(nullptr);
    }

    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

TEST_CASE("A-T3: the world is FROZEN on the ending screen (no sim, no movement)") {
    // The ending replaces gameplay: with a movement key HELD, the player must
    // NOT move, and an arbitrary number of frames must not advance the sim
    // (the early-return before the object tick / movement / sweep). This is
    // what makes the ending screen a steady, agency-only screen.
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();
    unsetenv("UMBRELLA_REDUCED_MOTION");
    unsetenv("UMBRELLA_LARGE_TARGETS");

    World world("", /*loadSprites=*/false);
    GameController controller{world};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const auto before = p->GetPosition();

    EnterEnding(world, nccu::SemesterState::Ending_D);
    in.Hold(Key::D);                       // hold "move right"
    for (int i = 0; i < 30; ++i) Frame(controller, in);
    const auto after = p->GetPosition();
    CHECK(after.x == doctest::Approx(before.x));   // frozen — did not move
    CHECK(after.y == doctest::Approx(before.y));
    // And no AppAction was requested by mere movement keys.
    CHECK(world.PendingAppAction() == World::AppAction::None);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
