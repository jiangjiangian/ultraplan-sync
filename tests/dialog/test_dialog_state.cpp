#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "game/controller/DialogChoiceApply.h"

using nccu::DialogState;
using nccu::DialogChoice;

TEST_CASE("Open with empty lines is a no-op (stays inactive)") {
    DialogState d;
    d.Open({});
    CHECK_FALSE(d.Active());
}

TEST_CASE("Open shows the FIRST line, not the second (show-then-advance)") {
    DialogState d;
    d.Open({"a", "b", "c"});
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "a");
}

TEST_CASE("Advance steps through lines then closes (no choices)") {
    DialogState d;
    d.Open({"a", "b"});
    CHECK(d.Advance() == nullptr);   // a -> b
    CHECK(d.CurrentLine() == "b");
    CHECK(d.Advance() == nullptr);   // b -> close
    CHECK_FALSE(d.Active());
}

TEST_CASE("CurrentLine on an inactive state is empty, never UB") {
    DialogState d;
    CHECK(d.CurrentLine().empty());
}

TEST_CASE("Lines then a choice: Advance past last line enters choice mode") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Advance() == nullptr);   // past "intro" -> choice mode
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
    CHECK(d.ChoiceCursor() == 0);
}

TEST_CASE("MoveChoice clamps; Advance at choice returns the picked one + closes") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    d.Advance();                 // -> choice mode
    d.MoveChoice(-1);            // clamp at 0
    CHECK(d.ChoiceCursor() == 0);
    d.MoveChoice(1);             // -> 1
    d.MoveChoice(1);             // clamp at 1 (size 2)
    CHECK(d.ChoiceCursor() == 1);
    const DialogChoice* picked = d.Advance();
    REQUIRE(picked != nullptr);
    CHECK(picked->label == "accept");
    CHECK(picked->karmaDelta == -5);
    CHECK(picked->setsFlag == nccu::kFlagScoldedSenior);
    CHECK_FALSE(d.Active());
}

TEST_CASE("Choice WITH nextLines: confirm plays the follow-up, then closes") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"help", 5, nccu::kFlagPromisedVictim, true, {"c0", "c1"}},
        {"ignore", -3, "", false}};
    d.Open({"intro"}, ch);
    CHECK(d.Advance() == nullptr);            // past "intro" -> choice mode
    CHECK(d.AtChoice());
    const DialogChoice* picked = d.Advance(); // confirm choice 0
    REQUIRE(picked != nullptr);
    CHECK(picked->label == "help");
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "c0");
    CHECK(d.Advance() == nullptr);            // c0 -> c1
    CHECK(d.CurrentLine() == "c1");
    CHECK(d.Advance() == nullptr);            // c1 -> close
    CHECK_FALSE(d.Active());
}

TEST_CASE("Returned choice pointer stays readable after Advance (UAF regression)") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    d.Advance();                 // -> choice mode
    d.MoveChoice(1);             // -> "accept"
    const DialogChoice* picked = d.Advance();   // confirms + Close()s
    REQUIRE(picked != nullptr);
    CHECK_FALSE(d.Active());                     // closed (no nextLines)
    // Pointer must still be valid AFTER Close cleared choices_.
    CHECK(picked->karmaDelta == -5);
    CHECK(picked->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(picked->flagValue == false);
}

TEST_CASE("ApplyDialogChoice mutates player karma and flag") {
    Player p{nccu::gfx::Vec2{0, 0}};
    const int before = p.GetKarma();
    nccu::DialogChoice c{"accept", -5, nccu::kFlagScoldedSenior, false};
    nccu::ApplyDialogChoice(p, c);
    CHECK(p.GetKarma() == before - 5);
    CHECK_FALSE(p.HasFlag(nccu::kFlagScoldedSenior));   // flagValue false -> ClearFlag
    nccu::DialogChoice c2{"help", 10, nccu::kFlagHelpedSenior, true};
    nccu::ApplyDialogChoice(p, c2);
    CHECK(p.GetKarma() == before - 5 + 10);
    CHECK(p.HasFlag(nccu::kFlagHelpedSenior));
}
