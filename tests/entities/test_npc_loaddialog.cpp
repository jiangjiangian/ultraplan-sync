#include "doctest/doctest.h"
#include "entities/NPC.h"
#include "entities/GameObject.h"
#include "state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <vector>

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

TEST_CASE("NPC exposes its lines via the GameObject::DialogLines virtual") {
    NPC npc(nccu::gfx::Vec2{0, 0}, {"hi", "there"}, true);
    GameObject& as_base = npc;
    const std::vector<std::string>* lines = as_base.DialogLines();
    REQUIRE(lines != nullptr);
    CHECK(lines->size() == 2);
    CHECK((*lines)[0] == "hi");
}
