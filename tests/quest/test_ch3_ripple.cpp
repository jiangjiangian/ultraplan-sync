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

TEST_CASE("ResolveOpenerSubState: Ch3 ripple routes by Ch1/Ch2 flags") {
    Player p = MakePlayer();

    // bookworm: (b) 未救回 by default, (a) Ch2 救回 once recovered.
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh3, p) == 1);
    p.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh3, p) == 0);

    // ta: (a) default, (c) on HelpedTA_Ch1.
    CHECK(nccu::ResolveOpenerSubState("ta", kCh3, p) == 0);
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh3, p) == 2);

    // victim: (b) no promise by default, (a) once promised.
    Player q = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("victim", kCh3, q) == 1);
    q.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", kCh3, q) == 0);

    // suit_senior: (a) default, (b) 物物交換鏈提示 on HelpedSenior.
    Player r = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh3, r) == 0);
    r.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh3, r) == 1);

    // Ch2 bookworm routing is untouched (separate state branch).
    Player s = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState(
              "bookworm", SemesterState::Chapter2_Midterms, s) == 0);
}

TEST_CASE("TryApplyCh3Ripple: ProfessorTrap -> -10 once per Ch3, key-independent") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    const int k0 = p.GetKarma();

    // No flag / wrong state -> no-op.
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, SemesterState::Chapter1_AddDrop);
    CHECK(p.GetKarma() == k0);

    p.SetFlag(nccu::kFlagHasProfessorTrap);
    // Wrong state still no-op even with the flag.
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, SemesterState::Chapter4_Finals);
    CHECK(p.GetKarma() == k0);

    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    CHECK(p.GetKarma() == k0 - 10);
    CHECK(p.HasFlag(nccu::kFlagCh3RippledProfTrap));
    nccu::TryApplyCh3Ripple(EventBus::Instance(), p, kCh3);
    CHECK(p.GetKarma() == k0 - 10);                 // once only

    // The Ch3 key is independent of the Ch2 one: a player who already
    // took the Ch2 -10 (Flag_Ch2Rippled_TA) still takes the Ch3 -10
    // (chapter3.md L329「獨立計算，不重複」).
    Player q = MakePlayer();
    const int qk = q.GetKarma();
    q.SetFlag(nccu::kFlagHasProfessorTrap);
    q.SetFlag(nccu::kFlagCh2RippledTA);                // Ch2 already debited
    nccu::TryApplyCh3Ripple(EventBus::Instance(), q, kCh3);
    CHECK(q.GetKarma() == qk - 10);
    EventBus::Instance().Clear();
}
