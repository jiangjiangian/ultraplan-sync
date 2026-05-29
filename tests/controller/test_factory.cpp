#include "doctest/doctest.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/Player.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/FragileUmbrella.h"
#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/CursedUmbrella.h"

TEST_CASE("Factory creates Player with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::Player, {100, 100});
    REQUIRE(obj != nullptr);
    auto* p = dynamic_cast<Player*>(obj.get());
    CHECK(p != nullptr);
}

TEST_CASE("Factory creates TrueUmbrella with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::TrueUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<TrueUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

TEST_CASE("Factory creates FragileUmbrella with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::FragileUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<FragileUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

TEST_CASE("Factory creates ProfessorTrapUmbrella with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<ProfessorTrapUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

TEST_CASE("Factory creates CursedUmbrella with correct dynamic type") {
    auto obj = GameObjectFactory::Create(ObjectType::CursedUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<CursedUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

TEST_CASE("CursedUmbrella BeClaimed bumps cursedTaint, defers karma to ApplyCursedTaintDecay") {
    // P2 (was F2): pickup itself is karma-neutral; the moral cost lands at
    // each subsequent chapter entry via ApplyCursedTaintDecay (-5 * taint).
    Player p({0, 0});
    int before = p.GetKarma();
    CursedUmbrella u({0, 0});
    u.BeClaimed(&p);
    CHECK(p.GetCursedTaint() == 1);              // taint incremented
    CHECK(p.GetKarma() == before);               // karma untouched at pickup
    CHECK(p.HasUmbrella() == true);              // bag still shows cursed row
}
