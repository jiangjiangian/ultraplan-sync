#include "doctest/doctest.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "TrueUmbrella.h"
#include "FragileUmbrella.h"
#include "ProfessorTrapUmbrella.h"
#include "CursedUmbrella.h"

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

TEST_CASE("CursedUmbrella applies karma penalty on beClaimed") {
    Player p({0, 0});
    int before = p.GetKarma();
    CursedUmbrella u({0, 0});
    u.beClaimed(&p);
    CHECK(p.GetKarma() == before - 30);  // F2: locked -30 big-event penalty
    CHECK(p.HasUmbrella() == true);
}
