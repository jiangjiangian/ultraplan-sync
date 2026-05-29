/**
 * @file test_ch3_umbrella_reveal.cpp
 * @brief 驗證 Ch3 真傘「給線索後才現身、位於體育館左側」，而 Ch4 真傘仍進場即生成於體育館後方。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/entities/TrueUmbrella.h"
#include "game/quest/Chapter3Quest.h"
#include "engine/events/EventBus.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

#include <cstddef>

using nccu::World;
using nccu::SemesterState;

// Ch3 的 TrueUmbrella 採「給線索後才現身」，並重新放到體育館左側，不再被體育館擋住。
//   • 進入 Ch3 時不會生成——只有在 C 系學姊揭露位置（Flag_KnowsUmbrellaLoc）後，
//     才透過 World::MaybeSpawnChapter3Umbrella 生成。
//   • 生成在 kChapter3UmbrellaPos，位於體育館左側（x=1320 < 體育館左緣 1493），
//     與 Ch4 藏在體育館後方的位置 (1640,375) 不同，後者維持不變。
//   • Ch4 仍在進場時就生成 TrueUmbrella，無條件，藏在體育館後方。

namespace {
std::size_t CountTrueUmbrellas(const World& w) {
    std::size_t n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const TrueUmbrella*>(o.get())) ++n;
    return n;
}
}  // namespace

// Ch3 真傘在揭露線索（Flag_KnowsUmbrellaLoc）前不生成，生成後位於體育館左側，且離開章節時隨名冊清除。
TEST_CASE("T5: Ch3 TrueUmbrella defers until Flag_KnowsUmbrellaLoc, then sweeps") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    // 依正式流程進入 Ch3。進場時沒有 TrueUmbrella（過去它會立刻生成在
    // (1640,375) 體育館內，造成被遮擋的問題）。
    w.Semester().Transition(SemesterState::Chapter3_SportsDay);
    w.RespawnChapterRoster(SemesterState::Chapter3_SportsDay);
    CHECK(CountTrueUmbrellas(w) == 0);

    // 沒有線索旗標時，延後生成每幀都是無操作。
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);

    // C 系學姊揭露後設下 Flag_KnowsUmbrellaLoc -> 傘現身一次，位於體育館左側。
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagKnowsUmbrellaLoc);
    CHECK(w.MaybeSpawnChapter3Umbrella());            // 這一幀生成
    CHECK(CountTrueUmbrellas(w) == 1);
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());       // 單次
    CHECK(CountTrueUmbrellas(w) == 1);

    // 它位於體育館左側（體育館左緣 x=1493）、不在體育館範圍內——也就是看得見。
    const TrueUmbrella* umb = nullptr;
    for (const auto& o : w.Objects())
        if (auto* t = dynamic_cast<const TrueUmbrella*>(o.get())) umb = t;
    REQUIRE(umb != nullptr);
    CHECK(umb->GetPosition().x < 1493.0f);            // 體育館左側
    CHECK(umb->GetPosition().x == nccu::kChapter3UmbrellaPos.x);
    CHECK(umb->GetPosition().y == nccu::kChapter3UmbrellaPos.y);

    // 離開 Ch3 時，傘會隨名冊一併清除（單次旗標重新武裝）。
    w.RespawnChapterRoster(SemesterState::Ending_A);
    CHECK(CountTrueUmbrellas(w) == 0);
    EventBus::Instance().Clear();
}

// Ch4 真傘仍在進場時就無條件生成在體育館後方（彩蛋路線），維持不變。
TEST_CASE("T5: Ch4 TrueUmbrella still spawns at entry, behind the gym (unchanged)") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);

    // Ch4 進場立即生成藏在體育館後方的傘（無條件）——通往 Ending A 的彩蛋備援路線，
    // 刻意保留在體育館內。
    w.Semester().Transition(SemesterState::Chapter4_Finals);
    w.RespawnChapterRoster(SemesterState::Chapter4_Finals);
    CHECK(CountTrueUmbrellas(w) == 1);

    const TrueUmbrella* umb = nullptr;
    for (const auto& o : w.Objects())
        if (auto* t = dynamic_cast<const TrueUmbrella*>(o.get())) umb = t;
    REQUIRE(umb != nullptr);
    CHECK(umb->GetPosition().x == 1640.0f);           // 體育館後方，維持原位
    CHECK(umb->GetPosition().y == 375.0f);

    // Ch4 的生成不受 Ch3 線索旗標影響。
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());       // 非 Ch3 時無操作
    CHECK(CountTrueUmbrellas(w) == 1);
    EventBus::Instance().Clear();
}

// MaybeSpawnChapter3Umbrella 在非 Ch3 章節一律無操作，即使已設線索旗標。
TEST_CASE("T5: MaybeSpawnChapter3Umbrella is a no-op outside Ch3") {
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    REQUIRE(w.GetPlayer() != nullptr);
    w.GetPlayer()->SetFlag(nccu::kFlagKnowsUmbrellaLoc);   // 旗標已設，但……

    // Ch1（建構子的預設狀態）：不生成。
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);

    // Ch2：不生成。
    w.Semester().Transition(SemesterState::Chapter2_Midterms);
    w.RespawnChapterRoster(SemesterState::Chapter2_Midterms);
    CHECK_FALSE(w.MaybeSpawnChapter3Umbrella());
    CHECK(CountTrueUmbrellas(w) == 0);
    EventBus::Instance().Clear();
}
