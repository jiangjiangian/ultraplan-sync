#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "game/controller/DialogChoiceApply.h"

using nccu::DialogState;
using nccu::DialogChoice;

/**
 * @file test_dialog_state.cpp
 * @brief 驗證 DialogState 狀態機：空台詞為空操作、先顯示首行再推進、逐行推進後關閉、
 *        台詞後進入選項模式、MoveChoice 裁切與確認、選項帶後續台詞，以及確認回傳的
 *        選項指標在關閉後仍可讀取，並驗證 ApplyDialogChoice 對玩家的影響。
 */

// 以空台詞 Open 是空操作（維持未啟用）。
TEST_CASE("Open with empty lines is a no-op (stays inactive)") {
    DialogState d;
    d.Open({});
    CHECK_FALSE(d.Active());
}

// Open 顯示第一行而非第二行（先顯示再推進）。
TEST_CASE("Open shows the FIRST line, not the second (show-then-advance)") {
    DialogState d;
    d.Open({"a", "b", "c"});
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "a");
}

// Advance 逐行推進後關閉（無選項）。
TEST_CASE("Advance steps through lines then closes (no choices)") {
    DialogState d;
    d.Open({"a", "b"});
    CHECK(d.Advance() == nullptr);   // a -> b
    CHECK(d.CurrentLine() == "b");
    CHECK(d.Advance() == nullptr);   // b -> 關閉
    CHECK_FALSE(d.Active());
}

// 在未啟用狀態下 CurrentLine 為空，絕不會是未定義行為。
TEST_CASE("CurrentLine on an inactive state is empty, never UB") {
    DialogState d;
    CHECK(d.CurrentLine().empty());
}

// 台詞接著選項：推進過最後一行後進入選項模式。
TEST_CASE("Lines then a choice: Advance past last line enters choice mode") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    CHECK(d.Active());
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Advance() == nullptr);   // 越過 "intro" -> 選項模式
    CHECK(d.AtChoice());
    CHECK(d.Choices().size() == 2);
    CHECK(d.ChoiceCursor() == 0);
}

// MoveChoice 會裁切游標；在選項處 Advance 回傳被選中者並關閉。
TEST_CASE("MoveChoice clamps; Advance at choice returns the picked one + closes") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    d.Advance();                 // -> 選項模式
    d.MoveChoice(-1);            // 裁切於 0
    CHECK(d.ChoiceCursor() == 0);
    d.MoveChoice(1);             // -> 1
    d.MoveChoice(1);             // 裁切於 1（共 2 個）
    CHECK(d.ChoiceCursor() == 1);
    const DialogChoice* picked = d.Advance();
    REQUIRE(picked != nullptr);
    CHECK(picked->label == "accept");
    CHECK(picked->karmaDelta == -5);
    CHECK(picked->setsFlag == nccu::kFlagScoldedSenior);
    CHECK_FALSE(d.Active());
}

// 帶後續台詞的選項：確認後播放後續台詞，再關閉。
TEST_CASE("Choice WITH nextLines: confirm plays the follow-up, then closes") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"help", 5, nccu::kFlagPromisedVictim, true, {"c0", "c1"}},
        {"ignore", -3, "", false}};
    d.Open({"intro"}, ch);
    CHECK(d.Advance() == nullptr);            // 越過 "intro" -> 選項模式
    CHECK(d.AtChoice());
    const DialogChoice* picked = d.Advance(); // 確認選項 0
    REQUIRE(picked != nullptr);
    CHECK(picked->label == "help");
    CHECK_FALSE(d.AtChoice());
    CHECK(d.Active());
    CHECK(d.CurrentLine() == "c0");
    CHECK(d.Advance() == nullptr);            // c0 -> c1
    CHECK(d.CurrentLine() == "c1");
    CHECK(d.Advance() == nullptr);            // c1 -> 關閉
    CHECK_FALSE(d.Active());
}

// 回傳的選項指標在 Advance（含關閉）後仍可讀取（避免 use-after-free）。
TEST_CASE("Returned choice pointer stays readable after Advance (UAF regression)") {
    DialogState d;
    std::vector<DialogChoice> ch{
        {"refuse", 0, "", false},
        {"accept", -5, nccu::kFlagScoldedSenior, false}};
    d.Open({"intro"}, ch);
    d.Advance();                 // -> 選項模式
    d.MoveChoice(1);             // -> "accept"
    const DialogChoice* picked = d.Advance();   // 確認並 Close()
    REQUIRE(picked != nullptr);
    CHECK_FALSE(d.Active());                     // 已關閉（無後續台詞）
    // 指標在 Close 清空 choices_ 之後仍必須有效。
    CHECK(picked->karmaDelta == -5);
    CHECK(picked->setsFlag == nccu::kFlagScoldedSenior);
    CHECK(picked->flagValue == false);
}

// ApplyDialogChoice 會改動玩家的 karma 與旗標。
TEST_CASE("ApplyDialogChoice mutates player karma and flag") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    const int before = p.GetKarma();
    nccu::DialogChoice c{"accept", -5, nccu::kFlagScoldedSenior, false};
    nccu::ApplyDialogChoice(p, c);
    CHECK(p.GetKarma() == before - 5);
    CHECK_FALSE(p.HasFlag(nccu::kFlagScoldedSenior));   // flagValue 為 false -> ClearFlag
    nccu::DialogChoice c2{"help", 10, nccu::kFlagHelpedSenior, true};
    nccu::ApplyDialogChoice(p, c2);
    CHECK(p.GetKarma() == before - 5 + 10);
    CHECK(p.HasFlag(nccu::kFlagHelpedSenior));
}
