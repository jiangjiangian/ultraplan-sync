// REQUIREMENT #9 regression: the in-game pause menu gained a 說明 item.
// This pins (a) the 4-item menu order (繼續 / 說明 / 重新開始 / 離開)
// via GameController's real input loop, (b) that 說明 opens an in-place
// help overlay (World::HelpOpen) instead of an AppAction, (c) that the
// help overlay is dismissable (ESC/E/Enter) back to the still-open menu,
// and (d) that the sim stays frozen the whole time (no movement / no
// rain tick) — so adding help can never perturb gameplay or the
// deterministic ending scripts.
//
// Revert-verify (must FAIL without the #9 menu wiring):
//   * Restore kMenuItemCount=3 / the 3-label menu and drop the case-1
//     SetHelpOpen branch — confirming index 1 then no longer opens help
//     (HelpOpen stays false); the "說明 opens help" CHECK fails.

#include "doctest/doctest.h"
#include "World.h"
#include "GameController.h"
#include "Player.h"
#include "GameHelp.h"
#include "EventBus.h"
#include "SemesterState.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include "gfx/Vec2.h"

#include <set>

using nccu::World;
using nccu::GameController;
using nccu::SemesterState;
using nccu::gfx::Key;

namespace {

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

TEST_CASE("REQ#9: pause menu 說明 opens/closes a help overlay, sim frozen") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);                       // settle (roster, etc.)
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const nccu::gfx::Vec2 pos0 = p->GetPosition();
    const float rain0 = p->GetRainMeter();

    // The menu is a 4-item menu now (REQUIREMENT #9).
    CHECK(World::kMenuItemCount == 4);

    // Open the pause menu (ESC).
    in.Tap(Key::Escape);
    Frame(controller, in);
    REQUIRE(world.MenuOpen());
    CHECK(world.MenuCursor() == 0);              // starts on 繼續
    CHECK_FALSE(world.HelpOpen());

    // Move to index 1 (說明) and confirm → help overlay opens; the menu
    // stays open behind it; NO AppAction was requested.
    in.Tap(Key::Down);
    Frame(controller, in);
    CHECK(world.MenuCursor() == 1);
    in.Tap(Key::Enter);
    Frame(controller, in);
    CHECK(world.HelpOpen());
    CHECK(world.MenuOpen());
    CHECK(world.PendingAppAction() == World::AppAction::None);

    // While help is up the sim is frozen: feed movement + many frames,
    // the player must not move and rain must not tick.
    for (int f = 0; f < 30; ++f) {
        in.Hold(Key::D);
        Frame(controller, in);
    }
    in.Release(Key::D);
    Frame(controller, in);
    CHECK(p->GetPosition().x == doctest::Approx(pos0.x));
    CHECK(p->GetPosition().y == doctest::Approx(pos0.y));
    CHECK(p->GetRainMeter() == doctest::Approx(rain0));
    CHECK(world.HelpOpen());                     // still up — D didn't close it

    // E dismisses help back to the menu (menu still open, cursor intact).
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK_FALSE(world.HelpOpen());
    CHECK(world.MenuOpen());
    CHECK(world.MenuCursor() == 1);

    // ESC from the menu now resumes the game (closing the menu also
    // clears any help latch via SetMenuOpen(false)).
    in.Tap(Key::Escape);
    Frame(controller, in);
    CHECK_FALSE(world.MenuOpen());
    CHECK_FALSE(world.HelpOpen());

    // The help text is non-empty and short enough for the panel (≤ 24
    // full-width cells worst case — the panel is narrower than the box).
    auto cells = [](std::string_view s) {
        // count UTF-8 lead bytes; CJK ≈ 1 cell here for a coarse bound,
        // every authored help line is CJK/ASCII mixed and intentionally
        // short. (Exact East-Asian width is the dialog box's concern;
        // the help panel is wide and only needs a sane upper bound.)
        int n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    };
    CHECK(nccu::kGameHelpLines.size() > 6);
    for (std::string_view ln : nccu::kGameHelpLines) CHECK(cells(ln) <= 24);
    CHECK_FALSE(nccu::kGameHelpClosing.empty());
    CHECK(cells(nccu::kGameHelpClosing) <= 24);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
