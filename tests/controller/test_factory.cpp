#include "doctest/doctest.h"
#include "controller/GameObjectFactory.h"
#include "entities/Player.h"
#include "entities/TrueUmbrella.h"
#include "entities/FragileUmbrella.h"
#include "entities/ProfessorTrapUmbrella.h"
#include "entities/CursedUmbrella.h"

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

TEST_CASE("CursedUmbrella beClaimed bumps cursedTaint, defers karma to ApplyCursedTaintDecay") {
    // P2 (was F2): pickup itself is karma-neutral; the moral cost lands at
    // each subsequent chapter entry via ApplyCursedTaintDecay (-5 * taint).
    Player p({0, 0});
    int before = p.GetKarma();
    CursedUmbrella u({0, 0});
    u.beClaimed(&p);
    CHECK(p.GetCursedTaint() == 1);              // taint incremented
    CHECK(p.GetKarma() == before);               // karma untouched at pickup
    CHECK(p.HasUmbrella() == true);              // bag still shows cursed row
}
