#include "doctest/doctest.h"
#include "quest/Chapter3Quest.h"
#include "dialog/DialogOpener.h"
#include "controller/EventBus.h"
#include "controller/EventWiring.h"
#include "entities/Player.h"
#include "state/SemesterStateMachine.h"
#include "gfx/Vec2.h"

#include <string>

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh3 = SemesterState::Chapter3_SportsDay;
}  // namespace

TEST_CASE("TryAdvanceCh3Trade: 物物交換鏈 advances one link per talk, in order") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // Out of order: talking B / C first is a no-op (no sausage yet).
    nccu::TryAdvanceCh3Trade(p, "loudspeaker_b", kCh3);
    nccu::TryAdvanceCh3Trade(p, "senior_c", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasLoudspeaker));
    CHECK_FALSE(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));
    CHECK(p.GetKarma() == k0);

    // Wrong state never advances.
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a",
                             SemesterState::Chapter1_AddDrop);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));

    // Link 1: A -> sausage, +3. Idempotent on a re-talk.
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);
    CHECK(p.HasFlag(nccu::kFlagHasSausage));
    CHECK(p.GetKarma() == k0 + 3);
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);
    CHECK(p.GetKarma() == k0 + 3);                  // not doubled

    // Link 2: B with sausage -> consume sausage, gain loudspeaker, +3.
    nccu::TryAdvanceCh3Trade(p, "loudspeaker_b", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));   // consumed
    CHECK(p.HasFlag(nccu::kFlagHasLoudspeaker));
    CHECK(p.GetKarma() == k0 + 6);
    nccu::TryAdvanceCh3Trade(p, "loudspeaker_b", kCh3);
    CHECK(p.GetKarma() == k0 + 6);

    // A must NOT re-give the sausage now the chain has moved past it.
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));
    CHECK(p.GetKarma() == k0 + 6);

    // Link 3: C with loudspeaker -> consume, learn umbrella loc, +5.
    nccu::TryAdvanceCh3Trade(p, "senior_c", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasLoudspeaker));   // consumed
    CHECK(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));
    CHECK(p.GetKarma() == k0 + 11);
    nccu::TryAdvanceCh3Trade(p, "senior_c", kCh3);
    CHECK(p.GetKarma() == k0 + 11);                 // chain fully spent
    EventBus::Instance().Clear();
}

TEST_CASE("ResolveOpenerSubState: Ch3 chain NPCs route (a)->(b) on their flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 0);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 0);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 0);

    p.SetFlag(nccu::kFlagHasSausage);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 0);

    p.ClearFlag(nccu::kFlagHasSausage);
    p.SetFlag(nccu::kFlagHasLoudspeaker);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 0);

    p.ClearFlag(nccu::kFlagHasLoudspeaker);
    p.SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK(nccu::ResolveOpenerSubState("vendor_sausage_a", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("loudspeaker_b", kCh3, p) == 1);
    CHECK(nccu::ResolveOpenerSubState("senior_c", kCh3, p) == 1);
}

TEST_CASE("Ch3 clear: claiming the 道具箱 TrueUmbrella -> Interlude returnTo Ch4") {
    EventBus::Instance().Clear();
    nccu::SemesterStateMachine m;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    m.Transition(SemesterState::Chapter3_SportsDay);
    REQUIRE(m.Current() == SemesterState::Chapter3_SportsDay);

    // Ch1-isomorphic: beClaimed publishes UmbrellaClaimed; the Ch3
    // sibling-if routes to the third market, returning to Ch4.
    EventBus::Instance().Publish(
        Event{EventType::UmbrellaClaimed, "TrueUmbrella"});
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter4_Finals);
    EventBus::Instance().Clear();
}
