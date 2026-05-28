#include "doctest/doctest.h"
#include "ui/ChapterToast.h"
#include "engine/events/EventBus.h"
#include "controller/EventWiring.h"
#include "state/InterludeExit.h"
#include "world/World.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::World;
using nccu::gfx::Vec2;

// H3 (cycle9): the Interlude exit zone (InterludeExit.h) was a silent
// position trigger — `InInterludeExitZone` returned true and the engine
// armed Flag_LeaveInterlude with zero feedback. Cycle9 added:
//   (a) an entry hint on Interlude arrival ("市集中央。逛完後往南離開"),
//       published from GameController right after the position reset.
//   (b) a once-per-visit "準備離開市集" toast the first frame the player
//       crosses into the south band. Latching is GameController-owned;
//       the helper MaybeAnnounceInterludeExit captures the lifecycle so
//       the test does not need the full Input/Renderer stack.

namespace {

// Subscribe a stack-stable std::vector<std::string> sink to the bus.
// The Subscription token is movable; the captured ref is to the caller's
// stack-local vector so the lambda stays valid for the whole test body.
[[nodiscard]] EventBus::Subscription
SubscribeToAll(std::vector<std::string>& texts) {
    return EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&texts](const Event& e) { texts.push_back(e.text); });
}

bool Contains(const std::vector<std::string>& v, const char* needle) {
    for (const auto& s : v) if (s == needle) return true;
    return false;
}

std::size_t Count(const std::vector<std::string>& v, const char* needle) {
    std::size_t n = 0;
    for (const auto& s : v) if (s == needle) ++n;
    return n;
}

} // namespace

TEST_CASE("InInterludeExitZone: south band detection still pins") {
    // Sanity pin so a future tweak to the zone constants can't quietly
    // void this whole suite. y=1910 is the cycle9 "first frame in band"
    // per the brief; the entry point (kInterludeEntry, y=1500) is NOT in
    // the zone so the player does not auto-trigger on arrival.
    CHECK(nccu::InInterludeExitZone(Vec2{500.0f, 1910.0f}));
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
}

TEST_CASE("MaybeAnnounceInterludeExit: publishes once, then idempotent") {
    EventBus::Instance().Clear();
    std::vector<std::string> texts;
    auto sub = SubscribeToAll(texts);
    bool latched = false;

    // First entry into the zone: publish.
    CHECK(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK(latched);
    CHECK(Contains(texts, nccu::kInterludeExitPrep));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    // Player straddles / re-enters the band before leaving the Interlude:
    // no second publish. This is the "no spam" guarantee — the helper
    // only fires once per latch cycle.
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    EventBus::Instance().Clear();
}

TEST_CASE("Interlude-visit latch lifecycle: reset on entry, fire on cross") {
    EventBus::Instance().Clear();
    std::vector<std::string> texts;
    auto sub = SubscribeToAll(texts);
    bool latched = false;

    // First visit: cross the band once.
    CHECK(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 1);

    // Player leaves the Interlude, then comes back: GameController resets
    // the latch in the Interlude-arrival branch. Mimic that reset and
    // verify the next zone entry fires again exactly once.
    latched = false;                                 // GC's reset
    CHECK(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 2);
    CHECK_FALSE(nccu::MaybeAnnounceInterludeExit(latched));
    CHECK(Count(texts, nccu::kInterludeExitPrep) == 2);

    EventBus::Instance().Clear();
}

TEST_CASE("Interlude arrival hint reaches the HUD subscriber") {
    // Full path: a ShowMessage publish containing the arrival hint lands
    // on World::HudMessage(), which the View reads. The hint string is
    // the contract — GameController's Interlude-arrival branch publishes
    // exactly this text right after RespawnChapterRoster.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    EventBus::Instance().Publish(
        Event{EventType::ShowMessage, nccu::kInterludeArrivalHint});
    CHECK(w.HudMessage() == nccu::kInterludeArrivalHint);

    EventBus::Instance().Clear();
}
