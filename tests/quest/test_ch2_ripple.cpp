/**
 * @file test_ch2_ripple.cpp
 * @brief 驗證 Ch2 漣漪：學長/助教開場依 Ch1 旗標路由，karma 漣漪每章只結算一次，且不與開場自動套用重複計分。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter2Quest.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;
}  // namespace

// Ch2 西裝學長的開場依 Ch1 漣漪旗標路由：預設 (a)、HelpedSenior→(b)、ScoldedSenior→(c)。
TEST_CASE("ResolveOpenerSubState：Ch2 西裝學長依 Ch1 漣漪旗標路由") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 0);  // (a)
    p.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 1);  // (b)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagScoldedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, q) == 2);  // (c)
}

// Ch2 助教開場：HelpedTA→(b)，但同時持有 ProfessorTrap 時 (c) 取代 (a)/(b)；Ch1 路由不受影響。
TEST_CASE("ResolveOpenerSubState：Ch2 助教 — ProfessorTrap 優先於 HelpedTA") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 0);           // (a)

    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 1);           // (b)

    // 兩者皆設：(c) 取代 (a)/(b)——ProfessorTrap 勝出。
    p.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 2);           // (c)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, q) == 2);

    // Ch1 路由不受影響（此分支限定在 Ch2）。
    Player r = MakePlayer();
    r.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState(
              "ta", SemesterState::Chapter1_AddDrop, r) == 0);
}

// Ch2 漣漪每章只結算一次：學長 +3、學長保持距離為 karma 中性、助教 ProfTrap -10、助教 HelpedTA 純資訊不計分。
TEST_CASE("TryApplyCh2Ripple：每章 Ch2 恰好結算一次 ±3 / -10") {
    SUBCASE("西裝學長 HelpedSenior -> +3 一次") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0 + 3);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);   // 再次對話
        CHECK(p.GetKarma() == k0 + 3);                     // 不加倍
    }
    SUBCASE("西裝學長 ScoldedSenior -> karma 中性，鍵只設一次") {
        // Ch1 (b) 的指正屬理性發言（+3），因此 Ch2 的「保持距離」漣漪不再用 -3
        // 把它扣回——此漣漪為 karma 中性（輕微尷尬，不是懲罰）。但仍會設下
        // 一次性鍵，讓這條支線只結算一次。
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagScoldedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0);                        // 不扣分
        CHECK(p.HasFlag(nccu::kFlagCh2RippledSuitSenior)); // 但已消耗一次
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0);
    }
    SUBCASE("助教 ProfessorTrap -> -10 一次") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHasProfessorTrap);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
    }
    SUBCASE("助教 僅 HelpedTA -> 不計 karma、不設鍵（純資訊漣漪）") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedTACh1);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0);
        CHECK_FALSE(p.HasFlag(nccu::kFlagCh2RippledTA));
    }
    SUBCASE("無漣漪旗標／章節不符／對象不符 -> 無操作") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);          // 沒旗標
        p.SetFlag(nccu::kFlagHelpedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior",
                                SemesterState::Chapter1_AddDrop);  // 非 Ch2
        nccu::TryApplyCh2Ripple(p, "bookworm", kCh2);              // 對象不符
        CHECK(p.GetKarma() == k0);
    }
}

// 開場的自動套用對已路由的漣漪不重複計分：開場加 0、實際 +3 只由 TryApplyCh2Ripple 結算一次。
TEST_CASE("不重複計分：開場自動套用對已路由的漣漪加 0") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagHelpedSenior);          // 路由 suit_senior -> (b)
    const int k0 = p.GetKarma();

    // 開場路由到 (b)；其一次性套用以 !HasFlag(Flag_HelpedSenior) 守門——
    // 旗標已持有——因此加 0 karma。
    nccu::OpenNpcDialog(d, p, "suit_senior", kCh2);
    CHECK(p.GetKarma() == k0);               // 開場未加任何分
    REQUIRE(d.Active());                     // (b) 以純台詞開啟

    // 只有 TryApplyCh2Ripple 會結算這 +3——總計恰好一次。
    nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
    CHECK(p.GetKarma() == k0 + 3);
}
