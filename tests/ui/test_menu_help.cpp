// REQUIREMENT #9 regression: the in-game pause menu gained a 說明 item.
// This pins (a) the 4-item menu order (繼續 / 說明 / 重新開始 / 離開)
// via GameController's real input loop, (b) that 說明 opens an in-place
// help overlay (World::HelpOpen) instead of an AppAction, (c) that the
// help overlay is dismissable (M/E/Enter) back to the still-open menu,
// and (d) that the sim stays frozen the whole time (no movement / no
// rain tick) — so adding help can never perturb gameplay or the
// deterministic ending scripts.
//
// Revert-verify (must FAIL without the #9 menu wiring):
//   * Restore kMenuItemCount=3 / the 3-label menu and drop the case-1
//     SetHelpOpen branch — confirming index 1 then no longer opens help
//     (HelpOpen stays false); the "說明 opens help" CHECK fails.
//   * Cycle 9.E.3 bumped kMenuItemCount from 4 → 6 by inserting the
//     減少動畫 / 擴大目標 toggle rows at indices 2 and 3. 說明 is still
//     index 1 (this test's invariant), so only the count CHECK below
//     needed updating; the index-1-opens-help path is unchanged.

#include "doctest/doctest.h"
#include "world/World.h"
#include "controller/GameController.h"
#include "entities/Player.h"
#include "ui/GameHelp.h"
#include "engine/events/EventBus.h"
#include "state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"
#include "engine/math/Vec2.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

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
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);                       // settle (roster, etc.)
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const nccu::gfx::Vec2 pos0 = p->GetPosition();
    const float rain0 = p->GetRainMeter();

    // The menu is a 6-item menu now (REQUIREMENT #9 + Cycle 9.E.3 added
    // 減少動畫 and 擴大目標 toggle rows between 說明 and 重新開始). The
    // 說明 item — what THIS test pins — remains at index 1.
    CHECK(World::kMenuItemCount == 6);

    // Open the pause menu (M — ESC is reserved for quitting the program).
    in.Tap(Key::M);
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

    // M from the menu now resumes the game (closing the menu also
    // clears any help latch via SetMenuOpen(false)).
    in.Tap(Key::M);
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

    // Cycle 9.E (audit H3 / D11 / SC 2.2.1): pin the pause-pauses-rain
    // hint into the help text so a player can discover the pause-as-
    // timing-extension affordance. The hint must live on a SINGLE line;
    // a line-by-line "contains both '暫停' and '雨壓力'" assertion is
    // key-agnostic (the menu key moved from ESC to M, but the pause
    // affordance is the same). A future copy-edit that rewrites the
    // wording but keeps both tokens on one line still passes; only
    // deleting the hint (or splitting it) fails.
    bool sawPauseRainHint = false;
    for (std::string_view ln : nccu::kGameHelpLines) {
        const bool hasPause = ln.find("暫停")   != std::string_view::npos;
        const bool hasRain  = ln.find("雨壓力") != std::string_view::npos;
        if (hasPause && hasRain) sawPauseRainHint = true;
    }
    CHECK(sawPauseRainHint);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// U2-T4: the 說明 overlay is PAGED (the help grew a 【道具須知】 section).
// Pin: (a) two pages, (b) the paged view + the closing line equal the flat
// kGameHelpLines (so the glyph-scan, which iterates the flat list, can't go
// stale), (c) the new economy/usage tips are present, (d) each page ends on
// a blank-or-content line within the panel's cell budget.
TEST_CASE("U2-T4: GameHelp is split into two consistent pages") {
    CHECK(nccu::kGameHelpPageCount == 2);
    REQUIRE(nccu::kGameHelpPages.size() == 2);

    // Flatten the pages, dropping the trailing closing line (which is the
    // last entry of page 2 AND kept as kGameHelpClosing). It must equal the
    // flat kGameHelpLines element-for-element.
    std::vector<std::string_view> paged;
    for (const auto page : nccu::kGameHelpPages)
        for (std::string_view ln : page) paged.push_back(ln);
    REQUIRE_FALSE(paged.empty());
    CHECK(paged.back() == nccu::kGameHelpClosing);
    paged.pop_back();
    REQUIRE(paged.size() == nccu::kGameHelpLines.size());
    for (std::size_t i = 0; i < paged.size(); ++i)
        CHECK(paged[i] == nccu::kGameHelpLines[i]);

    // Every page line within ~24 full-width cells (the panel budget).
    auto cells = [](std::string_view s) {
        int n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    };
    for (const auto page : nccu::kGameHelpPages)
        for (std::string_view ln : page) CHECK(cells(ln) <= 24);

    // The owner item-6 tips are present somewhere in the help.
    auto helpHas = [](std::string_view needle) {
        for (std::string_view ln : nccu::kGameHelpLines)
            if (ln.find(needle) != std::string_view::npos) return true;
        return false;
    };
    CHECK(helpHas("跨章節"));        // 金幣 persists across chapters
    CHECK(helpHas("清空"));          // other items wiped on market exit
    CHECK(helpHas("減緩雨量"));      // most items relieve rain
    CHECK(helpHas("自動減緩"));      // umbrella relieves rain automatically
}

// U2-T4: ←/→ flip the help page while the overlay is up; the index wraps and
// resets to page 0 on (re)open. Driven through GameController's real input
// loop so the wiring is pinned. The page is PURE UI state (World::HelpPage),
// NOT serialized (the harness emits no cursor/page), so a paged help leaves
// state.jsonl byte-identical — the sim is also frozen the whole time.
TEST_CASE("U2-T4: ←/→ page the 說明 overlay; page resets on open, sim frozen") {
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    GameController controller{world, EventBus::Instance()};
    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);                       // settle
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    const nccu::gfx::Vec2 pos0 = p->GetPosition();
    const float rain0 = p->GetRainMeter();

    // Open menu → 說明 (index 1) → Enter opens help on page 0.
    in.Tap(Key::M);          Frame(controller, in);
    in.Tap(Key::Down);       Frame(controller, in);
    in.Tap(Key::Enter);      Frame(controller, in);
    REQUIRE(world.HelpOpen());
    CHECK(world.HelpPage() == 0);                // opens on the first page

    // → advances to page 1; → again WRAPS back to 0 (2 pages).
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 1);
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 0);
    // ← wraps the other way: 0 → last page (1).
    in.Tap(Key::Left);       Frame(controller, in);
    CHECK(world.HelpPage() == 1);

    // The sim stayed frozen across all that paging (no move / no rain tick).
    CHECK(p->GetPosition().x == doctest::Approx(pos0.x));
    CHECK(p->GetPosition().y == doctest::Approx(pos0.y));
    CHECK(p->GetRainMeter() == doctest::Approx(rain0));

    // Close help (E) then reopen → back on page 0 (SetHelpOpen resets it).
    in.Tap(Key::E);          Frame(controller, in);
    CHECK_FALSE(world.HelpOpen());
    in.Tap(Key::Enter);      Frame(controller, in);   // cursor still on 說明
    REQUIRE(world.HelpOpen());
    CHECK(world.HelpPage() == 0);

    // Flip to page 1, then resume the game (M from the menu after dismissing
    // help) — SetMenuOpen(false) must also clear the help page latch.
    in.Tap(Key::Right);      Frame(controller, in);
    CHECK(world.HelpPage() == 1);
    in.Tap(Key::E);          Frame(controller, in);   // help → menu
    in.Tap(Key::M);          Frame(controller, in);   // menu → game
    CHECK_FALSE(world.MenuOpen());
    CHECK(world.HelpPage() == 0);                      // reset on resume

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
