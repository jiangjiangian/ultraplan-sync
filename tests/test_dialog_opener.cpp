#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"

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

TEST_CASE("3-arg OpenNpcDialog shop_auntie Ch1 is line-only (not in allowlist)") {
    DialogState d;
    nccu::OpenNpcDialog(d, "shop_auntie", SemesterState::Chapter1_AddDrop);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    int guard = 0;
    while (d.Active() && guard++ < 100) {
        CHECK_FALSE(d.AtChoice());     // never enters choice mode
        d.Advance();
    }
    CHECK_FALSE(d.Active());           // eventually closes
}
