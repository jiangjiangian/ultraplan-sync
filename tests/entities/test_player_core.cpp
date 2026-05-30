#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"

#include <string>

/**
 * @file test_player_core.cpp
 * @brief 驗證 Player 純資料狀態的四大支柱：金錢、會裁切的 karma 增減、旗標系統，
 *        以及雨量計累積與滿值後傳送回正門。此檔不涉及算繪／輸入。
 */

// 全新 Player 的預設值：karma 50、money 100、未淋雨、未持傘、無旗標。
TEST_CASE("Player 全新預設值：karma 50、money 100、未淋雨、未持傘、無旗標") {
    Player p({0, 0});
    CHECK(p.GetKarma() == 50);
    CHECK(p.GetMoney() == 100);
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK_FALSE(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedSenior));
    CHECK_FALSE(p.HasFlag(""));
}

// AddKarma 會裁切到 [-100, 100]，範圍內的增減則正常累積。
TEST_CASE("Player::AddKarma 裁切到 [-100, 100]，範圍內增減正常累積") {
    SUBCASE("範圍內的正向增量") {
        Player p({0, 0});
        p.AddKarma(30);
        CHECK(p.GetKarma() == 80);
    }
    SUBCASE("負向增量不會過度裁切到 -100") {
        Player p({0, 0});
        p.AddKarma(-100);
        CHECK(p.GetKarma() == -50);  // 50 + (-100) = -50，仍在範圍內
    }
    SUBCASE("正向溢出裁切到 100") {
        Player p({0, 0});
        p.AddKarma(200);
        CHECK(p.GetKarma() == 100);
    }
    SUBCASE("反覆大量負向裁切於 -100，絕不更低") {
        Player p({0, 0});
        p.AddKarma(-300);
        CHECK(p.GetKarma() == -100);
        p.AddKarma(-50);
        CHECK(p.GetKarma() == -100);
    }
}

// decreaseKarma(amount) 等價於 AddKarma(-amount)。
TEST_CASE("Player::decreaseKarma(amount) 等價於 AddKarma(-amount)") {
    Player p({0, 0});
    p.decreaseKarma(10);
    CHECK(p.GetKarma() == 40);

    // 對照一個以 AddKarma 驅動的分身，確認語意等價。
    Player twin({0, 0});
    twin.AddKarma(-10);
    CHECK(twin.GetKarma() == p.GetKarma());
}

// 金錢：AddMoney 累加，DeductMoney 會防止透支。
TEST_CASE("Player 金錢：AddMoney 累加，DeductMoney 防止透支") {
    Player p({0, 0});
    p.AddMoney(50);
    CHECK(p.GetMoney() == 150);

    const bool ok1 = p.DeductMoney(40);
    CHECK(ok1);
    CHECK(p.GetMoney() == 110);

    const bool ok2 = p.DeductMoney(200);
    CHECK_FALSE(ok2);
    CHECK(p.GetMoney() == 110);  // 透支：餘額不變
}

// 旗標：Set／Has／Clear 可往返；未設定的名稱回傳 false。
TEST_CASE("Player 旗標：Set／Has／Clear 可往返，未設定名稱回傳 false") {
    Player p({0, 0});
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedTACh1));

    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(p.HasFlag(nccu::kFlagHelpedTACh1));

    p.ClearFlag(nccu::kFlagHelpedTACh1);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedTACh1));

    // 對未設定的名稱呼叫 ClearFlag 是空操作（不丟例外，仍為 false）。
    p.ClearFlag("Flag_Never_Set");
    CHECK_FALSE(p.HasFlag("Flag_Never_Set"));
}

// ApplyRain 每秒累積 5 單位，持傘時為空操作，滿 100% 時傳送回正門並重設。
TEST_CASE("Player::ApplyRain 每秒累積 5 單位，持傘時為空操作，滿 100% 時傳送重生") {
    SUBCASE("無傘：曝雨 0.5 秒增加 2.5 單位") {
        Player p({0, 0});
        p.ApplyRain(0.5f);
        CHECK(p.GetRainMeter() == doctest::Approx(2.5f));
    }

    SUBCASE("已持傘：ApplyRain 為空操作") {
        Player p({0, 0});
        p.SetHasUmbrella(true);
        p.ApplyRain(1.0f);
        CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    }

    SUBCASE("雨量計 99 時曝雨 10 秒裁切到 100、傳送並重設") {
        EventBus::Instance().Clear();
        int hits = 0;
        std::string captured;
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [&](const Event& e) { hits++; captured = e.text; });

        Player p({1234.0f, 5678.0f});
        // 先用 19.8 秒曝雨灌到 99（5 * 19.8 = 99）。
        p.ApplyRain(19.8f);
        CHECK(p.GetRainMeter() == doctest::Approx(99.0f));

        p.ApplyRain(10.0f);  // 原本會推到 99 + 50 = 149，裁切為 100

        // 傳送的副作用：雨量計歸零、位置回到正門、發出帶企劃文字的 ShowMessage。
        CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
        CHECK(p.GetPosition().x == doctest::Approx(500.0f));
        CHECK(p.GetPosition().y == doctest::Approx(1860.0f));
        CHECK(hits == 1);
        CHECK(captured == "你淋成落湯雞了，被傳送回正門。半天就這樣過去了。");
    }
}
