// B5 regression: the four Ch2 reactive beats that USED to be inline
// `*（若 Flag_X = true）*` lines the DialogLoader silently dropped are
// now re-authored as genuine flag-gated SEPARATE substates in
// chapter2.md and routed by ResolveOpenerSubState. These cases prove
// each reactive line actually loads AND is reachable for the gating
// flag — and only for it. Revert-verified: reverting either the
// chapter2.md substate split or the DialogOpener Ch2 routing makes the
// routing/line CHECKs fail (the line is dropped / the default (a) opens
// instead).
#include "doctest/doctest.h"
#include "dialog/DialogOpener.h"
#include "dialog/DialogSource.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "quest/Chapter2Quest.h"
#include "gfx/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;

// Does any line of the substate vector equal `needle`?
bool SubHasLine(std::string_view npc, int sub, const std::string& needle) {
    for (const auto& e : nccu::dialog::Entries(npc, kCh2))
        if (e.subState == sub)
            for (const auto& l : e.lines)
                if (l == needle) return true;
    return false;
}
}  // namespace

TEST_CASE("B5: chapter2.md re-authored reactive substates load the line") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    // 學霸 (b) = cursed-umbrella cold reaction (subState 1).
    CHECK(SubHasLine("bookworm", 1, "……你今天感覺有點怪。"));
    // 福利社阿姨 (b) = ugly-umbrella recognition (subState 1).
    CHECK(SubHasLine("shop_auntie", 1, "你那把螢光綠的沒帶來嗎？辨識度那麼高。"));
    // 苦主 (c) = promise callback (subState 2).
    CHECK(SubHasLine("victim", 2, "你說過你會找——你真的還在找。"));
    // 苦主 (d) = ugly-umbrella recognition (subState 3).
    CHECK(SubHasLine("victim", 3, "那個螢光綠的是你的嗎？辨識度很高。"));
}

TEST_CASE("B5: Ch2 routing reaches each reactive substate only for its flag") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    SUBCASE("學霸: cursed-umbrella -> (b), else (a)") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 0);  // (a)
        p.SetFlag("Flag_TookCursedUmbrella");
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 1);  // (b)
        // Recovered still outranks the cursed variant.
        p.SetFlag("Flag_BookwormRecovered");
        CHECK(nccu::ResolveOpenerSubState("bookworm", kCh2, p) == 3);  // (d)
    }
    SUBCASE("福利社阿姨: ugly-umbrella -> (b), else (a)") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh2, p) == 0);
        p.SetFlag("Flag_BoughtUglyUmbrella");
        CHECK(nccu::ResolveOpenerSubState("shop_auntie", kCh2, p) == 1);
    }
    SUBCASE("苦主: promise -> (c); ugly-only -> (d); promise wins") {
        Player p = MakePlayer();
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 0);    // (a)
        p.SetFlag("Flag_BoughtUglyUmbrella");
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 3);    // (d)
        p.SetFlag("Flag_PromisedVictim");
        CHECK(nccu::ResolveOpenerSubState("victim", kCh2, p) == 2);    // (c)
    }
}

TEST_CASE("B5: OpenNpcDialog opens the reactive line end-to-end") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::dialog::Reload();

    SUBCASE("學霸 cursed: first line is the routed (b) opener, line-only") {
        Player p = MakePlayer();
        // A2 (hard-gate): the 學霸 is unreachable before the 圖書館管理員 is
        // met (Flag_MetLibrarian) — talked-to first he just slumps and a cue
        // points to the 櫃台. Meet her so this subcase exercises the genuine
        // cursed (b) reaction (which is gated behind the chain head, not a
        // first-contact bypass).
        p.SetFlag(nccu::kFlagMetLibrarian);
        p.SetFlag("Flag_TookCursedUmbrella");
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "bookworm", kCh2);
        REQUIRE(d.Active());
        CHECK_FALSE(d.AtChoice());
        CHECK(d.CurrentLine() == "……嗯？你說什麼。");        // (b) line 0
        bool sawCursed = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "……你今天感覺有點怪。") sawCursed = true;
            d.Advance();
        }
        CHECK(sawCursed);                                     // line displays
    }
    SUBCASE("苦主 promise: (c) recap reaches the callback line") {
        Player p = MakePlayer();
        p.SetFlag("Flag_PromisedVictim");
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "victim", kCh2);
        REQUIRE(d.Active());
        CHECK(d.CurrentLine() == "你也在圖書館備考嗎。");      // (c) line 0
        bool sawPromise = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "你說過你會找——你真的還在找。")
                sawPromise = true;
            d.Advance();
        }
        CHECK(sawPromise);
    }
    SUBCASE("no flag: 學霸 default (a) has NO cursed line") {
        Player p = MakePlayer();
        nccu::DialogState d;
        nccu::OpenNpcDialog(d, p, "bookworm", kCh2);
        REQUIRE(d.Active());
        bool sawCursed = false;
        for (int i = 0; i < 16 && d.Active(); ++i) {
            if (d.CurrentLine() == "……你今天感覺有點怪。") sawCursed = true;
            d.Advance();
        }
        CHECK_FALSE(sawCursed);   // cursed beat must NOT leak into (a)
    }
}
