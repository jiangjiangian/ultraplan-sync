#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "world/World.h"
#include "entities/Player.h"
#include "entities/TrueUmbrella.h"
#include "quest/Chapter3Quest.h"
#include "controller/EventBus.h"
#include "state/SemesterState.h"
#include "engine/math/Vec2.h"

#include <cstddef>

using nccu::World;
using nccu::SemesterState;

// T5 regression: the Ch3 TrueUmbrella is REVEAL-AFTER-CLUE and repositioned
// LEFT of the 體育館 so it is no longer occluded by the gym.
//   • It does NOT spawn at Ch3 entry — only after the C-系 學姊 reveals its
//     location (Flag_KnowsUmbrellaLoc), via World::MaybeSpawnChapter3Umbrella
//     (the sibling of MaybeSpawnChapter2Notes).
//   • It spawns at kChapter3UmbrellaPos, LEFT of the gym (x=1320 < gym left
//     edge 1493) — distinct from the Ch4 hidden-behind-gym spot (1640,375),
//     which is unchanged.
//   • Ch4 still spawns its TrueUmbrella at entry, ungated, behind the gym.

namespace {
std::size_t CountTrueUmbrellas(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const TrueUmbrella*>(o.get())) ++n;
    return n;
}
}  // namespace

TEST_CASE("T5: Ch3 TrueUmbrella defers until Flag_KnowsUmbrellaLoc, then sweeps") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    // Enter Ch3 the way production does. NO TrueUmbrella at entry (pre-T5 it
    // spawned immediately at (1640,375), inside the gym — the occlusion bug).
    w.Semester().Transition(SemesterState::Chapter3_SportsDay);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(CountTrueUmbrellas(w) == 0);

    // Without the clue flag the deferred spawn is a no-op every frame.
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);

    // The C-系 學姊 reveal sets Flag_KnowsUmbrellaLoc -> the umbrella appears
    // ONCE, LEFT of the gym.
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK(w.MaybeSpawnChapter3Umbrella());            // spawns this frame
    CHECK(CountTrueUmbrellas(w) == 1);
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());       // one-shot
    CHECK(CountTrueUmbrellas(w) == 1);

    // It is positioned LEFT of the 體育館 (gym left edge x=1493), not inside
    // the gym footprint — i.e. visible.
    const TrueUmbrella* umb = nullptr;
    for (const auto& o : w.Objects())
        if (auto* t = dynamic_cast<const TrueUmbrella*>(o.get())) umb = t;
    REQUIRE(umb != nullptr);
    CHECK(umb->GetPosition().x < 1493.0f);            // left of the gym
    CHECK(umb->GetPosition().x == nccu::kChapter3UmbrellaPos.x);
    CHECK(umb->GetPosition().y == nccu::kChapter3UmbrellaPos.y);

    // Leaving Ch3 sweeps the umbrella with the roster (re-arms the one-shot).
    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(CountTrueUmbrellas(w) == 0);
    EventBus::Instance().Clear();
}

TEST_CASE("T5: Ch4 TrueUmbrella still spawns at entry, behind the gym (unchanged)") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    // Ch4 entry spawns the hidden-behind-gym umbrella immediately (ungated) —
    // the easter-egg alternate route to Ending A, KEPT inside the gym.
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    w.RespawnChapterRoster(SemesterState::Chapter4_Finals);
    CHECK(CountTrueUmbrellas(w) == 1);

    const TrueUmbrella* umb = nullptr;
    for (const auto& o : w.Objects())
        if (auto* t = dynamic_cast<const TrueUmbrella*>(o.get())) umb = t;
    REQUIRE(umb != nullptr);
    CHECK(umb->GetPosition().x == 1640.0f);           // behind the gym, kept
    CHECK(umb->GetPosition().y == 375.0f);

    // The Ch4 spawn is NOT gated on the Ch3 clue flag.
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());       // no-op outside Ch3
    CHECK(CountTrueUmbrellas(w) == 1);
    EventBus::Instance().Clear();
}

TEST_CASE("T5: MaybeSpawnChapter3Umbrella is a no-op outside Ch3") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagKnowsUmbrellaLoc);   // flag set, but...

    // Ch1 (the ctor's default state): no spawn.
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);

    // Ch2: no spawn.
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);
    EventBus::Instance().Clear();
}
