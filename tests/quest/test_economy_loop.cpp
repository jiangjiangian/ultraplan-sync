#include "doctest/doctest.h"
#include "game/quest/ChapterPickups.h"
#include "game/entities/CashPickup.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;
using nccu::World;

// S5b-4 loop economy: money soft-cap, the "consumables 當章用完" wipe,
// the per-chapter CashPickup table, and proof the coins actually spawn.

TEST_CASE("Player::AddMoney clamps at the 300 soft cap, never the floor") {
    Player p{nccu::gfx::Vec2{0, 0}};
    REQUIRE(p.GetMoney() == 100);

    p.AddMoney(150);
    CHECK(p.GetMoney() == 250);
    p.AddMoney(100);                       // 350 -> clamped
    CHECK(p.GetMoney() == Player::kMoneySoftCap);
    CHECK(p.GetMoney() == 300);
    p.AddMoney(50);                        // already at cap, stays
    CHECK(p.GetMoney() == 300);

    // A negative amount still subtracts (the cap is a ceiling only).
    p.AddMoney(-120);
    CHECK(p.GetMoney() == 180);
    // DeductMoney is unaffected by the cap.
    CHECK(p.DeductMoney(80));
    CHECK(p.GetMoney() == 100);
}

TEST_CASE("Player::ClearConsumables wipes the whole inventory") {
    Player p{nccu::gfx::Vec2{0, 0}};
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    REQUIRE(p.ConsumableCount("HotPack") == 2);

    p.ClearConsumables();
    CHECK(p.ConsumableCount("HotPack")     == 0);
    CHECK(p.ConsumableCount("EnergyDrink") == 0);
    CHECK_FALSE(p.ConsumeOne("HotPack"));
}

TEST_CASE("ChapterPickups: Ch1 ~50, Ch2 anti-softlock >35, others empty") {
    const auto& ch1 = nccu::ChapterPickups(SemesterState::Chapter1_AddDrop);
    REQUIRE(ch1.size() == 5);
    int total = 0;
    for (const auto& p : ch1) total += p.value;
    CHECK(total == 50);

    // Ch2 funds the 35-money EnergyDrink for a player who arrives broke
    // (the anti-softlock floor chapter2.md §五.3 promises): 10+10+20 = 40.
    // The `> 35` check is the invariant — it must stay above the
    // 圖書館地下室自販機 price (ChapterVendors Chapter2Vendors) so a future
    // tweak can't silently re-introduce the wake-學霸 soft-lock.
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
