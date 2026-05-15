#include "doctest/doctest.h"
#include "DialogState.h"

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
        {"accept", -5, "Flag_ScoldedSenior", false}};
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
        {"accept", -5, "Flag_ScoldedSenior", false}};
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
    CHECK(picked->setsFlag == "Flag_ScoldedSenior");
    CHECK_FALSE(d.Active());
}
