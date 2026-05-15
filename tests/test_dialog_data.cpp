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

TEST_CASE("sub-block headings become choiceLabel (「」 span wins)") {
    // 苦主(b) heading is 玩家給予安慰（選擇「我去幫你追」） -> the quoted
    // span overrides; 西裝學長(b) 玩家拒絕拿傘 has no quote -> heading.
    const auto* v = find("victim", SemesterState::Chapter1_AddDrop, 1);
    REQUIRE(v != nullptr);
    CHECK(v->choiceLabel == std::string_view{"我去幫你追"});
    const auto* s = find("suit_senior", SemesterState::Chapter1_AddDrop, 1);
    REQUIRE(s != nullptr);
    CHECK(s->choiceLabel == std::string_view{"玩家拒絕拿傘"});
    // suit_senior (c): heading 玩家接受，取傘後交給學長 has no 「」 nor a
    // trailing （…）, so the cleaned heading is the label verbatim.
    const auto* s2 = find("suit_senior", SemesterState::Chapter1_AddDrop, 2);
    REQUIRE(s2 != nullptr);
    CHECK(s2->choiceLabel == std::string_view{"玩家接受，取傘後交給學長"});
    // Scene旁白 entries (SCENE_ID) are not (x) sub-blocks -> empty label.
    const auto* sc = find("__scene__", SemesterState::Chapter1_AddDrop, 0);
    REQUIRE(sc != nullptr);
    CHECK(sc->choiceLabel == std::string_view{""});
}

TEST_CASE("every entry has at least one line and a valid sub-state") {
    for (const auto& e : nccu::dialog::All()) {
        CHECK(e.lineCount >= 1);
        CHECK(e.subState >= 0);
        CHECK(e.subState <= 3);
    }
}
