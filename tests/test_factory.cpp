#include "doctest/doctest.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "TrueUmbrella.h"

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
