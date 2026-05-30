/**
 * @file test_ch4_routing.cpp
 * @brief 驗證 Ch4 各 NPC 開場子狀態的路由（依旗標與 karma 分支），且路由不得誤設旗標或誤改 karma。
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
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

// Ch4 西裝學長依 HelpedSenior 與 karma 分支到 (a)/(b)/(c)；無 HelpedSenior 時退化為 (a)。
TEST_CASE("ResolveOpenerSubState：Ch4 西裝學長依 HelpedSenior 與 karma 分支") {
    Player p = MakePlayer();
    // 無 HelpedSenior -> (a)（不出場是已知的省略，退化處理）。
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, p) == 0);

    p.SetFlag(nccu::kFlagHelpedSenior);
    p.AddKarma(100);                                  // 封頂約 100（>70）
    CHECK(p.GetKarma() > 70);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, p) == 1);  // (b)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagHelpedSenior);
    q.AddKarma(-100);                                 // < 30
    CHECK(q.GetKarma() < 30);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, q) == 2);  // (c)

    Player r = MakePlayer();                          // 預設 karma 約 50
    r.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, r) == 0);  // 中間值 -> (a)
}

// Ch4 助教在 (b)/(c) 同時成立時 (b) 優先；並驗證 bookworm／victim 的路由分支。
TEST_CASE("ResolveOpenerSubState：Ch4 助教 (b)/(c) 優先序，以及 bookworm/victim 路由") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 0);           // (a)
    p.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 2);           // (c)
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 1);  // (b) 優先於 (c)

    Player b = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh4, b) == 2);     // 未救
    b.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh4, b) == 1);     // 救回

    Player v = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 0);       // 釋懷
    v.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 1);       // 淡漠
    v.SetHasUmbrella(true);                                           // 傘已在手
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 0);       // 釋懷
}

// 路由到 Ch4 助教 (c) 時不得誤設 Flag_HelpedTA_Ch1，也不得誤改 karma。
TEST_CASE("OpenNpcDialog：Ch4 助教 (c) 路由不得誤設 HelpedTA") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagHasProfessorTrap);               // 路由助教 -> (c)
    const int k0 = p.GetKarma();

    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());                               // (c) 以純台詞開啟
    // chapter4.md 的優先順序敘述帶有 `Flag_HelpedTA_Ch1 = true`，解析器會
    // 將其記為 (c) 的 setsFlag。但自動套用是 Ch1 範圍限定的，因此路由到
    // Ch4 (c) 不得授予 HelpedTA_Ch1，也不得改動 karma。
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedTACh1));
    CHECK(p.GetKarma() == k0);
}

// Ch1 助教獎勵重述仍會生效（章節範圍守門完好）：驗證自動套用確有作用的不變量。
TEST_CASE("OpenNpcDialog：Ch1 助教獎勵重述仍會生效（守門完好）") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagFoundForm);                       // 路由 Ch1 助教 -> 1
    const int k0 = p.GetKarma();

    nccu::OpenNpcDialog(d, p, "ta", SemesterState::Chapter1_AddDrop);
    REQUIRE(d.Active());
    // Ch1 獎勵重述仍會觸發（Chapter1_AddDrop 範圍限定保住了這個機制）。
    // 這裡斷言的是不變量——自動套用確有作用（karma 改變或授予了旗標），
    // 而非特定的獎勵旗標名稱（Ch1 細節由 test_dialog_opener 負責）。
    CHECK((p.GetKarma() != k0 || p.HasFlag(nccu::kFlagHelpedTACh1)));
}
