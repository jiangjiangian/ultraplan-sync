#include "doctest/doctest.h"
#include "Player.h"

TEST_CASE("Player starts with karma 50 and no umbrella") {
    Player p({0, 0});
    CHECK(p.GetKarma() == 50);
    CHECK_FALSE(p.HasUmbrella());
}

TEST_CASE("Player::decreaseKarma subtracts the given amount") {
    Player p({0, 0});
    p.decreaseKarma(15);
    CHECK(p.GetKarma() == 35);
    p.decreaseKarma(40);
    CHECK(p.GetKarma() == -5);  // negative karma drives Bad Ending per design doc
}

TEST_CASE("Player::resetRainMeter zeroes the meter") {
    Player p({0, 0});
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    p.resetRainMeter();
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
}

TEST_CASE("Player::SetHasUmbrella toggles flag") {
    Player p({0, 0});
    CHECK_FALSE(p.HasUmbrella());
    p.SetHasUmbrella(true);
    CHECK(p.HasUmbrella());
}
