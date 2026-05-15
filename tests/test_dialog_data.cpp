#include "doctest/doctest.h"
#include "DialogData.h"
#include "SemesterState.h"
#include <string_view>

using nccu::SemesterState;

namespace {
const nccu::dialog::Entry* find(std::string_view npc,
                                SemesterState s, int sub) {
    for (const auto& e : nccu::dialog::All())
        if (e.npcId == npc && e.state == s && e.subState == sub)
            return &e;
    return nullptr;
}
}

TEST_CASE("Ch1 西裝學長 (a) has the 5 opening lines") {
    const auto* e = find("suit_senior",
                         SemesterState::Chapter1_AddDrop, 0);
    REQUIRE(e != nullptr);
    CHECK(e->lineCount == 5);
    CHECK(e->lines[0] == std::string_view{"欸，加退選也沒搶到嗎？"});
}

TEST_CASE("Ch1 西裝學長 (c) carries karma -5 and the scold flag") {
    const auto* e = find("suit_senior",
                         SemesterState::Chapter1_AddDrop, 2);
    REQUIRE(e != nullptr);
    CHECK(e->karmaDelta == -5);
    CHECK(e->setsFlag == std::string_view{"Flag_ScoldedSenior"});
    CHECK(e->flagValue == false);
}

TEST_CASE("every entry has at least one line and a valid sub-state") {
    for (const auto& e : nccu::dialog::All()) {
        CHECK(e.lineCount >= 1);
        CHECK(e.subState >= 0);
        CHECK(e.subState <= 3);
    }
}
