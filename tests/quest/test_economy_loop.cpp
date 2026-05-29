/**
 * @file test_economy_loop.cpp
 * @brief 驗證迴圈經濟：金錢軟上限、消耗品清空、各章金幣表，以及金幣確實生成於世界。
 */
#include "doctest/doctest.h"
#include "game/quest/ChapterPickups.h"
#include "game/entities/CashPickup.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;
using nccu::World;

// 迴圈經濟系統：金錢軟上限、「消耗品當章用完」的清空、各章 CashPickup 表，
// 以及驗證金幣確實會被生成到世界中。

// AddMoney 只在 300 軟上限封頂，不設下限；負數扣款與 DeductMoney 不受上限影響。
TEST_CASE("Player::AddMoney clamps at the 300 soft cap, never the floor") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    REQUIRE(p.GetMoney() == 100);

    p.AddMoney(150);
    CHECK(p.GetMoney() == 250);
    p.AddMoney(100);                       // 350 -> 封頂
    CHECK(p.GetMoney() == Player::kMoneySoftCap);
    CHECK(p.GetMoney() == 300);
    p.AddMoney(50);                        // 已達上限，維持不變
    CHECK(p.GetMoney() == 300);

    // 負數金額仍會扣減（上限只是天花板）。
    p.AddMoney(-120);
    CHECK(p.GetMoney() == 180);
    // DeductMoney 不受上限影響。
    CHECK(p.DeductMoney(80));
    CHECK(p.GetMoney() == 100);
}

// ClearConsumables 應清空整個消耗品背包（章節結束時的「當章用完」行為）。
TEST_CASE("Player::ClearConsumables wipes the whole inventory") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    REQUIRE(p.ConsumableCount("HotPack") == 2);

    p.ClearConsumables();
    CHECK(p.ConsumableCount("HotPack")     == 0);
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    CHECK_FALSE(p.ConsumeOne("HotPack"));
}

// 各章 CashPickup 金額表：Ch1 共 50、Ch2 共 40（防卡關下限 >35），其餘章節為空。
TEST_CASE("ChapterPickups: Ch1 ~50, Ch2 anti-softlock >35, others empty") {
    const auto& ch1 = nccu::ChapterPickups(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 5);
    int total = 0;
    for (const auto& p : ch1) total += p.value;
    CHECK(total == 50);

    // Ch2 必須足以讓身無分文進場的玩家買到 35 元的 EnergyDrink（防卡關下限）：
    // 10+10+20 = 40。`> 35` 是不變量——金額必須高於圖書館地下室自販機的售價，
    // 以免日後調整時悄悄重新引入「叫醒學霸」的卡關。
    const auto& ch2 = nccu::ChapterPickups(SemesterState::Chapter2_Midterms);
    REQUIRE(ch2.size() == 3);
    int ch2total = 0;
    for (const auto& p : ch2) ch2total += p.value;
    CHECK(ch2total == 40);
    CHECK(ch2total > 35);

    CHECK(nccu::ChapterPickups(SemesterState::Interlude_Market).empty());
    CHECK(nccu::ChapterPickups(SemesterState::Chapter3_SportsDay).empty());
    CHECK(nccu::ChapterPickups(SemesterState::Chapter4_Finals).empty());
}

// World 應確實生成 Ch1 的 5 枚金幣（總值 50），證明經濟迴圈有接上世界生成。
TEST_CASE("World actually spawns the Ch1 CashPickups (loop is wired)") {
    World w("", /*loadSprites=*/false);
    int coins = 0;
    int coinValue = 0;
    for (const auto& o : w.Objects()) {
        if (auto* c = dynamic_cast<CashPickup*>(o.get())) {
            ++coins;
            coinValue += c->Value();
        }
    }
    CHECK(coins == 5);
    CHECK(coinValue == 50);
}
