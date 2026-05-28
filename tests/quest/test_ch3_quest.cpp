#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "quest/Chapter3Quest.h"
#include "quest/ItemCatalog.h"
#include "dialog/DialogOpener.h"
#include "engine/events/EventBus.h"
#include "controller/EventWiring.h"
#include "entities/Player.h"
#include "state/SemesterStateMachine.h"
#include "world/World.h"
#include "engine/math/Vec2.h"

#include <cmath>
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

    // 校慶 lap gate: A won't trade until the player has run the 操場 lap.
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));   // no lap yet → no-op
    p.SetFlag(nccu::kFlagSportsLapDone);

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

// B2.4: the Ch3 物物交換鏈 must be VISIBLE in the bag and each trade must
// swap the row — the prior carried item disappears the instant it is traded
// (no stale row), the next appears, and 情報 (knowledge) is never a row.
TEST_CASE("B2.4: the Ch3 trade chain swaps bag rows cleanly (sausage -> 大聲公 -> none)") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    auto has = [](const std::vector<nccu::InventoryRow>& rows, const char* id) {
        for (const auto& r : rows) if (r.itemId == id) return true;
        return false;
    };

    // Pre-chain: neither carried item is in the bag.
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
    }

    // Link 1: get the sausage -> exactly the 香腸 row, no 大聲公 row.
    p.SetFlag(nccu::kFlagSportsLapDone);
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
    }

    // Link 2: trade sausage for 大聲公 -> the 香腸 row is GONE (consumed),
    // the 大聲公 row appears. The transfer leaves no stale row.
    nccu::TryAdvanceCh3Trade(p, "loudspeaker_b", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));    // no stale carried item
        CHECK(has(rows, nccu::kItemLoudspeaker));
    }

    // Link 3: trade 大聲公 for 情報 -> the 大聲公 row is GONE; 情報 is
    // knowledge, so the bag carries neither item now.
    nccu::TryAdvanceCh3Trade(p, "senior_c", kCh3);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK_FALSE(has(rows, nccu::kItemSausage));
        CHECK_FALSE(has(rows, nccu::kItemLoudspeaker));
        CHECK(p.HasFlag(nccu::kFlagKnowsUmbrellaLoc));  // knowledge gained
    }
    EventBus::Instance().Clear();
}

TEST_CASE("Item 4a: talking A pre-lap surfaces the 操場 lap hint in-fiction") {
    // The `!` on A (Ch3IndicatorVisible, pre-lap) leads the player to A; the
    // talk must then teach step 1 — run a lap — rather than silently doing
    // nothing. TryAdvanceCh3Trade publishes the redirect ShowMessage and
    // grants no sausage until Flag_SportsLapDone. Revert-verify: drop the
    // pre-lap `!` and the player has no cue to walk up to A in the first
    // place; drop the redirect branch and the talk is a silent no-op.
    EventBus::Instance().Clear();
    std::string lastMsg;
    int msgs = 0;
    EventBus::Instance().Subscribe(           // lives until the Clear() below
        EventType::ShowMessage,
        [&](const Event& e) { lastMsg = e.text; ++msgs; });

    Player p = MakePlayer();
    nccu::TryAdvanceCh3Trade(p, "vendor_sausage_a", kCh3);   // pre-lap talk
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasSausage));           // no sausage yet
    CHECK(msgs == 1);                                        // a hint fired
    CHECK(lastMsg.find("操場") != std::string::npos);        // points at the lap
    EventBus::Instance().Clear();
}

TEST_CASE("Ch3IndicatorVisible: the A->B->C `!` reveals one link at a time") {
    Player p = MakePlayer();
    // Item 4a (the "dead first step" fix): A is now the visible chain head
    // from chapter ENTRY, BEFORE the 操場 lap — pre-fix it was dark until
    // Flag_SportsLapDone, so the player saw NO `!` anywhere on entering Ch3
    // and wandered lost. Walking up to A pre-lap triggers TryAdvanceCh3Trade's
    // "先去操場跑一圈" redirect (teaching step 1 in-fiction); the same `!`
    // hands over the sausage after the lap.
    CHECK(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));   // lit pre-lap
    p.SetFlag(nccu::kFlagSportsLapDone);
    // After the lap: A still lit (chain head), the chain links B/C stay dark.
    CHECK(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));
    // A non-chain NPC is unaffected (always lit if it is a quest-giver).
    CHECK(nccu::Ch3IndicatorVisible("ta", p));

    // After trading at A: only B is lit.
    p.SetFlag(nccu::kFlagHasSausage);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("vendor_sausage_a", p));
    CHECK(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));

    // After trading at B: only C is lit.
    p.ClearFlag(nccu::kFlagHasSausage);
    p.SetFlag(nccu::kFlagHasLoudspeaker);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("loudspeaker_b", p));
    CHECK(nccu::Ch3IndicatorVisible("senior_c", p));

    // After C reveals the umbrella: the whole chain is dark (quest done).
    p.ClearFlag(nccu::kFlagHasLoudspeaker);
    p.SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK_FALSE(nccu::Ch3IndicatorVisible("senior_c", p));
}

TEST_CASE("World::UpdateSportsLap: a full 操場 lap sets Flag_SportsLapDone") {
    nccu::World w("", /*loadSprites=*/false);
    w.Semester().Transition(kCh3);
    REQUIRE(w.Semester().Current() == kCh3);
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE_FALSE(p->HasFlag(nccu::kFlagSportsLapDone));
    CHECK(w.SportsLapProgress() == doctest::Approx(0.0f));

    // Walk the player one full circle around the track centre.
    const float cx = nccu::kSportsTrackCx, cy = nccu::kSportsTrackCy;
    const float r = nccu::kSportsTrackR;
    constexpr float kTwoPi = 6.2831853f;
    for (int i = 0; i <= 40; ++i) {
        const float a = static_cast<float>(i) * (kTwoPi / 40.0f);
        p->SetPosition(nccu::gfx::Vec2{cx + r * std::cos(a),
                                       cy + r * std::sin(a)});
        w.UpdateSportsLap();
    }
    CHECK(p->HasFlag(nccu::kFlagSportsLapDone));
    CHECK(w.SportsLapProgress() == doctest::Approx(1.0f));

    // Standing in the centre (off the track band) never sweeps the lap.
    nccu::World w2("", /*loadSprites=*/false);
    w2.Semester().Transition(kCh3);
    Player* p2 = w2.GetPlayer();
    for (int i = 0; i < 40; ++i) {
        p2->SetPosition(nccu::gfx::Vec2{cx, cy});   // dead centre
        w2.UpdateSportsLap();
    }
    CHECK_FALSE(p2->HasFlag(nccu::kFlagSportsLapDone));
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
