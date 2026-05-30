#include "doctest/doctest.h"
#include "game/controller/GameObjectFactory.h"
#include "game/entities/Player.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/FragileUmbrella.h"
#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/CursedUmbrella.h"

/**
 * @file test_factory.cpp
 * @brief 驗證 GameObjectFactory 依 ObjectType 建出正確的動態型別，並檢查
 *        CursedUmbrella 撿取時的業力（karma）延遲扣減語意。
 */

// Factory 建出的 Player 動態型別正確。
TEST_CASE("Factory 建出的 Player 動態型別正確") {
    auto obj = GameObjectFactory::Create(ObjectType::Player, {100, 100});
    REQUIRE(obj != nullptr);
    auto* p = dynamic_cast<Player*>(obj.get());
    CHECK(p != nullptr);
}

// Factory 建出的 TrueUmbrella 動態型別正確。
TEST_CASE("Factory 建出的 TrueUmbrella 動態型別正確") {
    auto obj = GameObjectFactory::Create(ObjectType::TrueUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<TrueUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

// Factory 建出的 FragileUmbrella 動態型別正確。
TEST_CASE("Factory 建出的 FragileUmbrella 動態型別正確") {
    auto obj = GameObjectFactory::Create(ObjectType::FragileUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<FragileUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

// Factory 建出的 ProfessorTrapUmbrella 動態型別正確。
TEST_CASE("Factory 建出的 ProfessorTrapUmbrella 動態型別正確") {
    auto obj = GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<ProfessorTrapUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

// Factory 建出的 CursedUmbrella 動態型別正確。
TEST_CASE("Factory 建出的 CursedUmbrella 動態型別正確") {
    auto obj = GameObjectFactory::Create(ObjectType::CursedUmbrella, {200, 200});
    REQUIRE(obj != nullptr);
    auto* u = dynamic_cast<CursedUmbrella*>(obj.get());
    CHECK(u != nullptr);
}

// CursedUmbrella::BeClaimed 只增加 cursedTaint，業力延遲到後續章節進入時由
// ApplyCursedTaintDecay 才扣減。
TEST_CASE("CursedUmbrella BeClaimed 只增加 cursedTaint，業力延遲到 ApplyCursedTaintDecay") {
    // 撿取本身不影響業力；道德代價在每次進入新章節時透過 ApplyCursedTaintDecay
    //（-5 * taint）才落地。
    Player p({0, 0});
    int before = p.GetKarma();
    CursedUmbrella u({0, 0});
    u.BeClaimed(&p);
    CHECK(p.GetCursedTaint() == 1);              // taint 遞增
    CHECK(p.GetKarma() == before);               // 撿取時業力不動
    CHECK(p.HasUmbrella() == true);              // 背包仍顯示詛咒傘那列
}
