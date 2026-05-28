// REQUIREMENT #4 regression: buying from a Vendor must NEVER be forced.
#include "quest/Flags.h"
// Every vendor menu carries a trailing "不買" (decline) choice; picking
// it closes the conversation with ZERO economy mutation — no money
// deducted, no consumable added, no item.setsFlag set, no PickupAcquired
// EventBus event. This drives the REAL GameController::Update() loop
// through the same gfx::Input choke point the harness/I5 test use, so it
// exercises the exact production decline path, not a unit shim.
//
// Revert-verify (this test must FAIL without the production fix):
//   * Drop the trailing decline DialogChoice in OpenVendorMenu (so the
//     menu only has stock lines) — the "menu always has a decline tail"
//     CHECKs fail.
//   * OR remove the `stockIdx >= stockN` decline guard in the confirm
//     branch (so the decline index falls through to Vendor::TryBuy with
//     an out-of-range index) — TryBuy(out-of-range) returns false so no
//     money moves, BUT the menu-size/label CHECKs still pin the option's
//     presence; the decisive guard is the "decline never calls TryBuy"
//     contract: with the guard gone a decline on a SINGLE-stock vendor
//     (cursor moved to the decline entry) still must not buy — which it
//     wouldn't here only by luck of the bounds check, so the primary
//     revert signal is the appended-choice CHECK pair below.

#include "doctest/doctest.h"
#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "quest/ChapterVendors.h"
#include "engine/events/EventBus.h"
#include "engine/core/GameObject.h"
#include "state/SemesterState.h"
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

const GameObject* FindVendor(const World& w) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->IsVendor()) return u.get();
    return nullptr;
}

}  // namespace

// The Ch4 集英樓 vendor sells the ugly umbrella (price 100, setsFlag
// Flag_BoughtUglyUmbrella → Ending C). Open the menu, move the cursor
// onto the trailing "不買" entry, confirm: the dialog closes and NOTHING
// changes — purse, inventory, flag, EventBus all untouched. This is the
// "no forced purchase" guarantee for REQUIREMENT #4.
TEST_CASE("REQ#4: declining a vendor purchase mutates nothing") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int pickupHits = 0;
    EventBus::Instance().Subscribe(EventType::PickupAcquired,
        [&](const Event&) { ++pickupHits; });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                              // roster -> Ch4

    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(300);
    const int money0 = p->GetMoney();
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    REQUIRE(p->ConsumableCount("UglyUmbrella") == 0);

    p->SetPosition(nccu::gfx::Vec2{vend->GetPosition().x - 8.0f,
                                   vend->GetPosition().y});

    in.Tap(Key::E);                                     // open the buy menu
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());

    // The menu is stock(1) + the trailing decline. Last choice is "不買".
    const std::size_t n = world.Dialog().Choices().size();
    REQUIRE(n == 2);
    CHECK(world.Dialog().Choices().back().label == "先不買，謝謝");
    CHECK(world.Dialog().Choices().back().setsFlag.empty());
    CHECK(world.Dialog().Choices().back().karmaDelta == 0);

    // Move the cursor down onto the LAST entry (the decline) and confirm.
    for (std::size_t i = 0; i + 1 < n; ++i) {
        in.Tap(Key::Down);
        Frame(controller, in);
    }
    CHECK(world.Dialog().ChoiceCursor() == static_cast<int>(n) - 1);

    const int karma0 = p->GetKarma();
    in.Tap(Key::E);                                     // confirm DECLINE
    Frame(controller, in);

    // Nothing happened: dialog closed, purse/flag/inventory/event/karma
    // all exactly as before — a purchase was NOT forced.
    CHECK_FALSE(world.Dialog().Active());
    CHECK(p->GetMoney() == money0);
    CHECK(p->GetKarma() == karma0);
    CHECK_FALSE(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    CHECK(p->ConsumableCount("UglyUmbrella") == 0);
    CHECK(pickupHits == 0);

    // And the vendor is still usable afterwards: re-open + actually buy
    // this time (cursor defaults back to the stock line) — declining did
    // not break the stall.
    p->SetPosition(nccu::gfx::Vec2{vend->GetPosition().x - 8.0f,
                                   vend->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());
    CHECK(world.Dialog().ChoiceCursor() == 0);          // back on the buy line
    in.Tap(Key::E);                                     // confirm BUY
    Frame(controller, in);
    CHECK(p->GetMoney() == money0 - 100);
    CHECK(p->HasFlag(nccu::kFlagBoughtUglyUmbrella));
    CHECK(pickupHits == 1);

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
