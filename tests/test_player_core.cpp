#include "doctest/doctest.h"
#include "Player.h"
#include "EventBus.h"

#include <string>

// Covers the four pillars of Player's pure-data state: money, karma deltas
// with clamping, the Flag system, and rain-meter accumulation with gate
// respawn. Rendering / input are not exercised here.

TEST_CASE("Player: fresh defaults — karma 50, money 100, dry, umbrella-less, no flags") {
    Player p({0, 0});
    CHECK(p.GetKarma() == 50);
    CHECK(p.GetMoney() == 100);
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK_FALSE(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag("Flag_HelpedSenior"));
    CHECK_FALSE(p.HasFlag(""));
}

TEST_CASE("Player::AddKarma clamps to [-100, 100] and ordinary deltas accumulate") {
    SUBCASE("positive delta within range") {
        Player p({0, 0});
        p.AddKarma(30);
        CHECK(p.GetKarma() == 80);
    }
    SUBCASE("negative delta does not over-clamp to -100") {
        Player p({0, 0});
        p.AddKarma(-100);
        CHECK(p.GetKarma() == -50);  // 50 + (-100) = -50, in range
    }
    SUBCASE("positive overflow clamps to 100") {
        Player p({0, 0});
        p.AddKarma(200);
        CHECK(p.GetKarma() == 100);
    }
    SUBCASE("repeated large negatives clamp at -100, never below") {
        Player p({0, 0});
        p.AddKarma(-300);
        CHECK(p.GetKarma() == -100);
        p.AddKarma(-50);
        CHECK(p.GetKarma() == -100);
    }
}

TEST_CASE("Player::decreaseKarma(amount) is equivalent to AddKarma(-amount)") {
    Player p({0, 0});
    p.decreaseKarma(10);
    CHECK(p.GetKarma() == 40);

    // Compare against an AddKarma-driven twin to confirm semantic equivalence.
    Player twin({0, 0});
    twin.AddKarma(-10);
    CHECK(twin.GetKarma() == p.GetKarma());
}

TEST_CASE("Player money: AddMoney accrues, DeductMoney guards against overdraft") {
    Player p({0, 0});
    p.AddMoney(50);
    CHECK(p.GetMoney() == 150);

    const bool ok1 = p.DeductMoney(40);
    CHECK(ok1);
    CHECK(p.GetMoney() == 110);

    const bool ok2 = p.DeductMoney(200);
    CHECK_FALSE(ok2);
    CHECK(p.GetMoney() == 110);  // overdraft: balance untouched
}

TEST_CASE("Player flags: Set / Has / Clear round-trip; unset names return false") {
    Player p({0, 0});
    CHECK_FALSE(p.HasFlag("Flag_HelpedTA_Ch1"));

    p.SetFlag("Flag_HelpedTA_Ch1");
    CHECK(p.HasFlag("Flag_HelpedTA_Ch1"));

    p.ClearFlag("Flag_HelpedTA_Ch1");
    CHECK_FALSE(p.HasFlag("Flag_HelpedTA_Ch1"));

    // ClearFlag on an unset name is a no-op (no throw, still false).
    p.ClearFlag("Flag_Never_Set");
    CHECK_FALSE(p.HasFlag("Flag_Never_Set"));
}

TEST_CASE("Player::ApplyRain accumulates 5 units/sec, no-ops with umbrella, respawns at 100%") {
    SUBCASE("no umbrella: 0.5s of exposure adds 2.5 units") {
        Player p({0, 0});
        p.ApplyRain(0.5f);
        CHECK(p.GetRainMeter() == doctest::Approx(2.5f));
    }

    SUBCASE("umbrella equipped: ApplyRain is a no-op") {
        Player p({0, 0});
        p.SetHasUmbrella(true);
        p.ApplyRain(1.0f);
        CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    }

    SUBCASE("rain meter at 99 with 10s of exposure clamps to 100, respawns, resets") {
        EventBus::Instance().Clear();
        int hits = 0;
        std::string captured;
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [&](const Event& e) { hits++; captured = e.text; });

        Player p({1234.0f, 5678.0f});
        // Pre-soak to 99 via 19.8s of exposure (5 * 19.8 = 99).
        p.ApplyRain(19.8f);
        CHECK(p.GetRainMeter() == doctest::Approx(99.0f));

        p.ApplyRain(10.0f);  // would push to 99 + 50 = 149, clamps to 100

        // Respawn side-effects: rainMeter zeroed, position back at gate,
        // ShowMessage event delivered with the design-doc text.
        CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
        CHECK(p.GetPosition().x == doctest::Approx(500.0f));
        CHECK(p.GetPosition().y == doctest::Approx(1860.0f));
        CHECK(hits == 1);
        CHECK(captured == "你淋成落湯雞了，被傳送回正門。半天就這樣過去了。");
    }
}
