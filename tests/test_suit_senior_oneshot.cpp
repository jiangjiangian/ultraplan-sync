#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"
#include "DialogSource.h"
#include "Player.h"
#include "SemesterState.h"
#include "gfx/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;
using nccu::gfx::Vec2;

// C.3(b): 西裝學長 is the ripple-critical choice-opener. Once a choice
// has been committed (GameController sets Flag_SuitSeniorChoiceMade),
// re-talking must recap line-only — no branch menu — so the player can
// never stack mutually-exclusive ripple flags ((d) Flag_HelpedSenior
// then (c) Flag_ScoldedSenior). shop_auntie / victim stay re-enterable.

namespace {

// Drives a DialogState forward to its terminal: either it lands at a
// choice menu (returns true) or it closes line-only (returns false).
bool DriveToChoiceOrClose(nccu::DialogState& dlg) {
    for (int guard = 0; guard < 64 && dlg.Active(); ++guard) {
        if (dlg.AtChoice()) return true;
        dlg.Advance();
    }
    return dlg.AtChoice();
}

}  // namespace

TEST_CASE("C.3(b): first 西裝學長 talk presents the branch menu") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};

    nccu::OpenNpcDialog(dlg, p, "suit_senior",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());
    CHECK(dlg.NpcId() == "suit_senior");
    CHECK(DriveToChoiceOrClose(dlg));            // reaches the menu
    CHECK_FALSE(dlg.Choices().empty());          // (b)/(c)/(d) present
}

TEST_CASE("C.3(b): after the choice flag, 西裝學長 recaps line-only") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};
    const int karma0 = p.GetKarma();

    // GameController sets this when a suit_senior choice is confirmed.
    p.SetFlag("Flag_SuitSeniorChoiceMade");

    nccu::OpenNpcDialog(dlg, p, "suit_senior",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());                        // opener still shows
    CHECK(dlg.NpcId() == "suit_senior");
    CHECK_FALSE(DriveToChoiceOrClose(dlg));       // never reaches a menu
    CHECK(dlg.Choices().empty());                 // line-only recap
    CHECK_FALSE(dlg.Active());                    // closed past last line
    CHECK(p.GetKarma() == karma0);                // no re-applied karma
}

TEST_CASE("C.3(b): the guard is scoped to 西裝學長 — 苦主 still branches") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::DialogState dlg;
    Player p{Vec2{0, 0}};

    // The suit_senior guard flag must not leak onto other choice-openers.
    p.SetFlag("Flag_SuitSeniorChoiceMade");

    nccu::OpenNpcDialog(dlg, p, "victim",
                        SemesterState::Chapter1_AddDrop);
    REQUIRE(dlg.Active());
    CHECK(DriveToChoiceOrClose(dlg));             // 苦主 still presents A/B
    CHECK_FALSE(dlg.Choices().empty());
}
