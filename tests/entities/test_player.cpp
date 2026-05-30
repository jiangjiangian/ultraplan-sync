/**
 * @file test_player.cpp
 * @brief 驗證 Player 基本狀態：初始 karma、減 karma、清空雨量計、雨傘旗標切換。
 */
#include "doctest/doctest.h"
#include "game/entities/Player.h"

// 玩家初始 karma 為 50 且未持傘。
TEST_CASE("Player 初始 karma 為 50 且未持傘") {
    Player p({0, 0});
    CHECK(p.GetKarma() == 50);
    CHECK_FALSE(p.HasUmbrella());
}

// decreaseKarma 會扣掉指定數值，且 karma 可為負。
TEST_CASE("Player::decreaseKarma 扣掉指定數值（karma 可為負）") {
    Player p({0, 0});
    p.decreaseKarma(15);
    CHECK(p.GetKarma() == 35);
    p.decreaseKarma(40);
    CHECK(p.GetKarma() == -5);  // 負 karma 會導向 Bad Ending
}

// resetRainMeter 會把雨量計歸零。
TEST_CASE("Player::resetRainMeter 將雨量計歸零") {
    Player p({0, 0});
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    p.resetRainMeter();
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
}

// SetHasUmbrella 會切換持傘旗標。
TEST_CASE("Player::SetHasUmbrella 切換持傘旗標") {
    Player p({0, 0});
    CHECK_FALSE(p.HasUmbrella());
    p.SetHasUmbrella(true);
    CHECK(p.HasUmbrella());
}
