/**
 * @file test_ch3_ripple.cpp
 * @brief 驗證 Ch3 漣漪：各 NPC 開場依 Ch1/Ch2 旗標路由，且 ProfessorTrap 的 -10 每章一次、與 Ch2 鍵獨立。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter3Quest.h"
#include "game/dialog/DialogOpener.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh3 = SemesterState::Chapter3_SportsDay;
}  // namespace

// Ch3 各 NPC 的開場子狀態依前章旗標路由（bookworm／ta／victim／suit_senior），且 Ch2 路由不受影響。
TEST_CASE("ResolveOpenerSubState：Ch3 漣漪依 Ch1/Ch2 旗標路由") {
    Player p = MakePlayer();

    // bookworm：預設為 (b) 未救回，Ch2 救回後變 (a)。
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh3, p) == 1);
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh3, p) == 0);

    // ta：預設 (a)，有 HelpedTA_Ch1 時為 (c)。
    CHECK(nccu::ResolveOpenerSubState("ta", kCh3, p) == 0);
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh3, p) == 2);

    // victim：預設為 (b) 未承諾，承諾後變 (a)。
    Player q = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("victim", kCh3, q) == 1);
    q.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", kCh3, q) == 0);

    // suit_senior：預設 (a)，有 HelpedSenior 時為 (b) 物物交換鏈提示。
    Player r = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh3, r) == 0);
    r.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh3, r) == 1);

    // Ch2 的 bookworm 路由不受影響（屬獨立的狀態分支）。
    Player s = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, s) == 0);
}

// ProfessorTrap 在 Ch3 造成 -10，每章只一次；且此鍵與 Ch2 的扣分鍵彼此獨立計算。
TEST_CASE("TryApplyCh3Ripple：ProfessorTrap 每章 Ch3 -10 一次，且鍵彼此獨立") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // 沒旗標／章節不符 -> 無操作。
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, SemesterState::Chapter1_AddDrop);
    CHECK(p.GetKarma() == k0);

    p.SetFlag(nccu::kFlagHasProfessorTrap);
    // 即使有旗標，章節不符仍為無操作。
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, SemesterState::Chapter4_Finals);
    CHECK(p.GetKarma() == k0);

    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    CHECK(p.GetKarma() == k0 - 10);
    CHECK(p.HasFlag(nccu::kFlagCh3RippledProfTrap));
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    CHECK(p.GetKarma() == k0 - 10);                 // 只一次

    // Ch3 的扣分鍵與 Ch2 的獨立：即使玩家已在 Ch2 被扣 -10
    //（Flag_Ch2Rippled_TA），在 Ch3 仍會再被扣 -10（獨立計算、不重複互抵）。
    Player q = MakePlayer();
    const int qk = q.GetKarma();
    q.SetFlag(nccu::kFlagHasProfessorTrap);
    q.SetFlag(nccu::kFlagCh2RippledTA);                // Ch2 already debited
    nccu::TryApplyCh3Ripple(EventBus::Instance(), q, kCh3);
    CHECK(q.GetKarma() == qk - 10);
    EventBus::Instance().Clear();
}
