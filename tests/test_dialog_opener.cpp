#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"
#include "Player.h"

using nccu::DialogState;
using nccu::SemesterState;

TEST_CASE("OpenNpcDialogSub loads victim Ch1 (a) opener, line-only") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "victim", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "……我的傘不見了。");
    for (int i = 0; i < 4; ++i) CHECK(d.Advance() == nullptr);
    CHECK(d.CurrentLine() == "你也是被偷的嗎？");
    CHECK(d.Advance() == nullptr);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialogSub unknown npcId leaves dialog inactive") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "nobody", SemesterState::Chapter1_AddDrop, 0);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialogSub suit_senior Ch1 (a) opener first line") {
    DialogState d;
    nccu::OpenNpcDialogSub(d, "suit_senior",
                           SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
}

TEST_CASE("3-arg OpenNpcDialog victim Ch1: opener + 2 choices, (b) plays") {
    DialogState d;
    nccu::OpenNpcDialog(d, "victim", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘不見了。");      // (a) opener line 0
    // Step through the 5 opener lines into choice mode.
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    REQUIRE(d.Choices().size() == 2);
    // Table order: subState 1 (b) first, subState 2 (c) second.
    CHECK(d.Choices()[0].label == "我去幫你追");
    CHECK(d.Choices()[1].label == "玩家無視走過");
    // Pick the help branch (index 0) -> its (b) consequence plays.
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "我去幫你追");
    CHECK(c->setsFlag == "Flag_PromisedVictim");
    CHECK(c->flagValue == true);
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "真的？謝謝你……");          // (b) line 0
    while (d.Active()) d.Advance();                       // exhaust -> close
    CHECK_FALSE(d.Active());
}

TEST_CASE("3-arg OpenNpcDialog shop_auntie Ch1: opener + buy-umbrella choice") {
    DialogState d;
    nccu::OpenNpcDialog(d, "shop_auntie", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    // Step through the 4 (a) opener lines into choice mode.
    for (int i = 0; i < 4; ++i) d.Advance();
    CHECK(d.AtChoice());
    REQUIRE(d.Choices().size() == 2);
    // Table order: subState 1 first, subState 2 second.
    CHECK(d.Choices()[0].label == "玩家詢問雨傘");
    CHECK(d.Choices()[1].label == "玩家購買醜綠傘後");
    // C.1 (pre-approved): the Ch1 阿姨 buy branch now seeds
    // Flag_KnowsUglyUmbrella, NOT Flag_BoughtUglyUmbrella. The real
    // purchase (→ Ending C) moved to the Ch4 集英樓 Vendor; the Ch1
    // buy is now a 伏筆 seed, not an instant ending trigger.
    d.MoveChoice(1);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "玩家購買醜綠傘後");
    CHECK(c->setsFlag == "Flag_KnowsUglyUmbrella");
    CHECK(c->flagValue == true);
    while (d.Active()) d.Advance();    // exhaust -> close
    CHECK_FALSE(d.Active());
}

TEST_CASE("ResolveOpenerSubState: ta gated by fetch-quest flags") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 0);   // fresh
    p.SetFlag("Flag_FoundForm");
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // reward
    p.SetFlag("Flag_HelpedTA_Ch1");
    CHECK(nccu::ResolveOpenerSubState("ta", Ch1, p) == 1);   // stays 1
}

TEST_CASE("ResolveOpenerSubState: victim recap gated by promise flag") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 0);  // fresh
    p.SetFlag("Flag_PromisedVictim");
    CHECK(nccu::ResolveOpenerSubState("victim", Ch1, p) == 1);  // recap
}

TEST_CASE("ResolveOpenerSubState: non-quest NPC always subState 0") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
    p.SetFlag("Flag_FoundForm");
    p.SetFlag("Flag_PromisedVictim");
    CHECK(nccu::ResolveOpenerSubState("bookworm", Ch1, p) == 0);
}

TEST_CASE("Player overload: ta reward applies karma/flag exactly once") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_FoundForm");
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");  // ta sub-1 line 0
    CHECK(p.HasFlag("Flag_HelpedTA_Ch1"));
    CHECK(p.GetKarma() == k0 + 5);
    // Re-open with the SAME player -> apply-once guard skips (no double).
    DialogState d2;
    nccu::OpenNpcDialog(d2, p, "ta", Ch1);
    CHECK(d2.Active());
    CHECK(d2.CurrentLine() == "謝謝你……那份表格要是不見我真的完了。");
    CHECK(p.GetKarma() == k0 + 5);
}

TEST_CASE("Player overload: ta with no flag is the line-only intro") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p2{nccu::gfx::Vec2{0, 0}};
    const int k0 = p2.GetKarma();
    DialogState d2;
    nccu::OpenNpcDialog(d2, p2, "ta", Ch1);
    CHECK(d2.Active());
    CHECK_FALSE(d2.AtChoice());
    CHECK(d2.CurrentLine() == "同學，加退選截止了，現在不受理。");  // ta sub-0 line 0
    CHECK(p2.GetKarma() == k0);
    CHECK_FALSE(p2.HasFlag("Flag_HelpedTA_Ch1"));
}

TEST_CASE("Player overload: victim no flag still opens the 1b-2 choice") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "……我的傘不見了。");      // (a) opener line 0
    for (int i = 0; i < 5; ++i) d.Advance();
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
}

TEST_CASE("Player overload: victim with promise flag is line-only recap") {
    const auto Ch1 = SemesterState::Chapter1_AddDrop;
    Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_PromisedVictim");
    const int k0 = p.GetKarma();
    DialogState d;
    nccu::OpenNpcDialog(d, p, "victim", Ch1);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "真的？謝謝你……");        // victim sub-1 line 0
    CHECK(p.GetKarma() == k0);   // +5 was the choice's job, flag preset -> skip
}
