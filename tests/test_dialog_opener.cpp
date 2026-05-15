#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"

using nccu::DialogState;
using nccu::SemesterState;

TEST_CASE("OpenNpcDialog loads victim Ch1 (a) opener, line-only") {
    DialogState d;
    nccu::OpenNpcDialog(d, "victim", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.CurrentLine() == "……我的傘不見了。");
    for (int i = 0; i < 4; ++i) CHECK(d.Advance() == nullptr);
    CHECK(d.CurrentLine() == "你也是被偷的嗎？");
    CHECK(d.Advance() == nullptr);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialog unknown npcId leaves dialog inactive") {
    DialogState d;
    nccu::OpenNpcDialog(d, "nobody", SemesterState::Chapter1_AddDrop, 0);
    CHECK_FALSE(d.Active());
}

TEST_CASE("OpenNpcDialog suit_senior Ch1 (a) opener first line") {
    DialogState d;
    nccu::OpenNpcDialog(d, "suit_senior", SemesterState::Chapter1_AddDrop, 0);
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "欸，加退選也沒搶到嗎？");
}
