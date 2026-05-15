#include "doctest/doctest.h"
#include "NPC.h"
#include "SemesterState.h"
#include "gfx/Vec2.h"

using nccu::SemesterState;

TEST_CASE("LoadDialog pulls suit_senior Ch1 (a) opening lines") {
    NPC npc(nccu::gfx::Vec2{0, 0}, {"placeholder"}, true);
    npc.LoadDialog("suit_senior", SemesterState::Chapter1_AddDrop, 0);
    CHECK(npc.DialogLineCount() == 5);
    CHECK(npc.CurrentLineText()
          == std::string{"欸，加退選也沒搶到嗎？"});
}

TEST_CASE("LoadDialog on a missing key leaves dialog empty") {
    NPC npc(nccu::gfx::Vec2{0, 0}, {"x"}, false);
    npc.LoadDialog("nobody", SemesterState::Ending_A, 0);
    CHECK(npc.DialogLineCount() == 0);
}
