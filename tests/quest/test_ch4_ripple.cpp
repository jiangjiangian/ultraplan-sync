/**
 * @file test_ch4_ripple.cpp
 * @brief 驗證 Ch4 各 NPC 的 karma 漣漪：依前章旗標各自加減、每章只結算一次、且彼此獨立。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter4Quest.h"
#include "game/dialog/DialogOpener.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

// 學長 +10 只在 (b) 路徑（HelpedSenior 且 karma>70）發生，且每章只一次。
TEST_CASE("TryApplyCh4Ripple: 學長 +10 only on the (b) route, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // 沒有 HelpedSenior -> 即使高 karma 也沒有 +10。
    p.AddKarma(40);                          // 約 90（>70），但無旗標
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 40);

    p.SetFlag(nccu::kFlagHelpedSenior);
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 50);          // +10 生效
    nccu::TryApplyCh4Ripple(p, "suit_senior", kCh4);
    CHECK(p.GetKarma() == k0 + 50);          // 只結算一次

    // karma 未 > 70 -> 沒有 +10。
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagHelpedSenior);          // 預設 karma 約 50（<70）
    const int qk = q.GetKarma();
    nccu::TryApplyCh4Ripple(q, "suit_senior", kCh4);
    CHECK(q.GetKarma() == qk);
}

// 學霸 +5 在 BookwormRecovered 時發生，且每章只一次。
TEST_CASE("TryApplyCh4Ripple: 學霸 +5 on BookwormRecovered, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0);               // no flag
    p.SetFlag(nccu::kFlagBookwormRecovered);
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0 + 5);
    nccu::TryApplyCh4Ripple(p, "bookworm", kCh4);
    CHECK(p.GetKarma() == k0 + 5);
}

// 助教的 +10（HelpedTA）與 -15（ProfTrap）彼此獨立，可同時生效（淨 -5），且各自只一次。
TEST_CASE("TryApplyCh4Ripple: 助教 +10 / -15 are INDEPENDENT (L235)") {
    SUBCASE("HelpedTA only -> +10") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedTACh1);
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 + 10);
    }
    SUBCASE("ProfTrap only -> -15") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHasProfessorTrap);
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 15);
    }
    SUBCASE("both -> +10 AND -15 (net -5), each once") {
        Player p = MakePlayer(); const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedTACh1);
        p.SetFlag(nccu::kFlagHasProfessorTrap);
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 5);
        nccu::TryApplyCh4Ripple(p, "ta", kCh4);
        CHECK(p.GetKarma() == k0 - 5);       // 兩個結算鍵都已消耗
    }
}

// 福利社阿姨的 Ch1→Ch4 漣漪：Ch1 阿姨 (d) 請咖啡選項種下的
// Flag_BoughtCoffeeForAuntie_Ch1，在 Ch4 兌現為 +3 的「直接情報」回呼，只一次。
TEST_CASE("B3 TryApplyCh4Ripple: 福利社阿姨 +3 on coffee flag, once") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    // Ch1 沒請咖啡 -> 走間接情報路徑，沒有 +3。
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0);
    CHECK_FALSE(p.HasFlag(nccu::kFlagCh4RippledAuntie));

    p.SetFlag(nccu::kFlagBoughtCoffeeForAuntie);
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0 + 3);                 // 情分回呼生效
    CHECK(p.HasFlag(nccu::kFlagCh4RippledAuntie));
    nccu::TryApplyCh4Ripple(p, "shop_auntie", kCh4);
    CHECK(p.GetKarma() == k0 + 3);                 // 只結算一次

    // 章節不符 -> 即使有旗標也是無操作。
    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagBoughtCoffeeForAuntie);
    const int qk = q.GetKarma();
    nccu::TryApplyCh4Ripple(q, "shop_auntie",
                            SemesterState::Chapter1_AddDrop);
    CHECK(q.GetKarma() == qk);
}

// Ch4 阿姨對話依 Ch1 咖啡旗標路由：買過咖啡走 (a) 直接情報，否則走 (d) 間接情報。
TEST_CASE("B3 ResolveOpenerSubState: Ch4 shop_auntie coffee routing") {
    Player p = MakePlayer();
    // Ch1 沒請咖啡 -> (d) 間接情報。
    CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh4, p) == 3);
    p.SetFlag(nccu::kFlagBoughtCoffeeForAuntie);
    // 有咖啡情分 -> (a) 直接情報。
    CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh4, p) == 0);
}

// 章節不符或對象不符時，Ch4 漣漪一律為無操作。
TEST_CASE("TryApplyCh4Ripple: wrong state / wrong npc -> no-op") {
    Player p = MakePlayer();
    const int k0 = p.GetKarma();
    p.SetFlag(nccu::kFlagHelpedSenior);
    p.AddKarma(40);
    nccu::TryApplyCh4Ripple(p, "suit_senior",
                            SemesterState::Chapter1_AddDrop);
    nccu::TryApplyCh4Ripple(p, "victim", kCh4);
    CHECK(p.GetKarma() == k0 + 40);          // unchanged
}
