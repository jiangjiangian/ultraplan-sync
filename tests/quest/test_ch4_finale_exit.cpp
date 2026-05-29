// Item 1c regression ("能退出的選項"): the Ch4 助教 結算 menu must carry a
#include "game/quest/Flags.h"
// trailing no-commit exit. Picking it closes the conversation with ZERO
// state change — NO Flag_TaFinaleChoiceMade, NO Flag_ConsoledTA, NO karma
// applied — so the player can walk off and re-approach 助教 to decide the
// finale later. This is the moral-choice analogue of the vendor "先不買"
// decline (test_vendor_decline.cpp) and the gate against a soft-lock: the
// finale menu self-locks via Flag_TaFinaleChoiceMade, so an accidental
// commit would foreclose Ending A forever.
//
// Driven through the REAL GameController::Update() loop via the gfx::Input
// choke point (the exact production confirm path), not a unit shim — the
// decisive guard is `exitChoice` in GameController.cpp's choice-confirm
// branch.
//
// Revert-verify (this test MUST FAIL without the production fix):
//   * Drop the trailing kDialogExitLabel choice in DialogOpener's 助教
//     menu — the menu-tail CHECKs fail.
//   * OR remove the `!exitChoice &&` guard on the Flag_TaFinaleChoiceMade
//     set in GameController — declining then sets the self-lock flag, so
//     CHECK_FALSE(HasFlag(nccu::kFlagTaFinaleChoiceMade)) fails and the menu
//     is no longer re-presentable.

#include "doctest/doctest.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
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

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// Mash E until the dialog reaches choice mode (opener lines exhausted).
void AdvanceToChoice(nccu::GameController& c, TestInput& in, World& w) {
    for (int f = 0; f < 24 && !w.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(c, in);
    }
}

}  // namespace

TEST_CASE("1c: declining the 助教 finale (我再想想…) mutates nothing & re-opens") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                              // settle Ch4 roster

    const GameObject* ta = FindNpc(world, "ta");
    REQUIRE(ta != nullptr);
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);

    // Pre-state baseline: high karma + the Ch4-reclaimed TrueUmbrella so a
    // 體諒 commit WOULD reach Ending A — proving the exit really withholds
    // that outcome, not that the conditions were merely unmet.
    p->AddKarma(40);                                    // ~90
    p->SetFlag(nccu::kFlagHasTrueUmbrella);
    const int karma0 = p->GetKarma();
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagTaFinaleChoiceMade));
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagConsoledTA));

    // Walk onto the 助教 and open the 結算 menu.
    p->SetPosition(nccu::gfx::Vec2{ta->GetPosition().x - 8.0f,
                                   ta->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    AdvanceToChoice(controller, in, world);
    REQUIRE(world.Dialog().AtChoice());

    // The menu is 體諒 / 質問 / exit — the exit is LAST.
    const std::size_t n = world.Dialog().Choices().size();
    REQUIRE(n == 3);
    CHECK(world.Dialog().Choices().back().label == nccu::kDialogExitLabel);

    // Move the cursor down onto the exit entry and confirm.
    for (std::size_t i = 0; i + 1 < n; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    CHECK(world.Dialog().ChoiceCursor() == static_cast<int>(n) - 1);
    in.Tap(Key::E);                                     // confirm DECLINE
    Frame(controller, in);

    // Nothing committed: dialog closed, the finale is NOT made, no karma
    // moved, Ending A did NOT fire, the spine is still in Ch4.
    CHECK_FALSE(world.Dialog().Active());
    CHECK_FALSE(p->HasFlag(nccu::kFlagTaFinaleChoiceMade));
    CHECK_FALSE(p->HasFlag(nccu::kFlagConsoledTA));
    CHECK(p->GetKarma() == karma0);
    CHECK(world.Semester().Current() == SemesterState::Chapter4_Finals);

    // And the finale is still re-approachable — re-open and the menu (with
    // the same three options) comes back, so the decision was only deferred.
    p->SetPosition(nccu::gfx::Vec2{ta->GetPosition().x - 8.0f,
                                   ta->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    AdvanceToChoice(controller, in, world);
    REQUIRE(world.Dialog().AtChoice());
    CHECK(world.Dialog().Choices().size() == 3);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
