#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "dialog/DialogOpener.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "state/EndingGate.h"
#include "engine/events/EventBus.h"
#include "entities/Player.h"
#include "state/SemesterStateMachine.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;

// Step opener lines until the code-constructed 結算 menu appears.
void StepToChoice(nccu::DialogState& d) {
    int guard = 0;
    while (d.Active() && !d.AtChoice() && guard++ < 64) d.Advance();
}
}  // namespace

TEST_CASE("S5e-2d: Ch4 助教 結算 presents the code-constructed 體諒/質問 menu") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;

    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());
    StepToChoice(d);
    REQUIRE(d.AtChoice());
    // 1c: the menu now carries a trailing no-commit exit (index 2). 體諒/質問
    // keep indices 0/1 (appended LAST, vendor-decline contract), so the
    // routing/karma CHECKs below are unchanged.
    REQUIRE(d.Choices().size() == 3);
    CHECK(d.Choices()[0].label == "體諒助教的辛勞");
    CHECK(d.Choices()[1].label == "質問／強硬索回");
    CHECK(d.Choices()[2].label == nccu::kDialogExitLabel);
    // The exit option carries NO side effect at all.
    CHECK(d.Choices()[2].karmaDelta == 0);
    CHECK(d.Choices()[2].setsFlag.empty());

    // 體諒 (index 0): +15 karma, sets Flag_ConsoledTA (Ending A key).
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "體諒助教的辛勞");
    CHECK(c->karmaDelta == 15);
    CHECK(c->setsFlag == nccu::kFlagConsoledTA);
    CHECK(c->flagValue == true);
}

TEST_CASE("S5e-2d: 質問 branch is the -5 / no-flag path") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    StepToChoice(d);
    REQUIRE(d.AtChoice());
    d.MoveChoice(1);
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "質問／強硬索回");
    CHECK(c->karmaDelta == -5);
    CHECK(c->setsFlag.empty());
}

TEST_CASE("S5e-2d: Flag_TaFinaleChoiceMade -> line-only recap (one-shot)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);          // choice already made
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());
    // Recap never re-presents the menu — stepping just exhausts lines.
    int guard = 0;
    while (d.Active() && guard++ < 64) {
        CHECK_FALSE(d.AtChoice());
        d.Advance();
    }
    CHECK_FALSE(d.Active());
}

TEST_CASE("S5e-2d: the 體諒 choice closes the Ending A path end-to-end") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.AddKarma(40);                                // ~90, pre-體諒
    p.SetFlag(nccu::kFlagHasTrueUmbrella);             // re-claimed Ch4 True

    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    StepToChoice(d);
    REQUIRE(d.AtChoice());
    const nccu::DialogChoice* c = d.Advance();     // 體諒 (index 0)
    REQUIRE(c != nullptr);
    // Replicate GameController's ApplyDialogChoice (free fn, not exposed).
    p.AddKarma(c->karmaDelta);
    if (!c->setsFlag.empty() && c->flagValue) p.SetFlag(c->setsFlag);
    CHECK(p.GetKarma() > 80);
    CHECK(p.HasFlag(nccu::kFlagConsoledTA));

    nccu::SemesterStateMachine m;
    m.Transition(kCh4);
    // G2: the 體諒 choice queued its closing nextLines, so `d` is STILL
    // active here — and CheckEndingGates now DEFERS behind an active dialog
    // (the player reads the 拿回你的傘 beat first). Confirm it defers, then
    // close the box and re-poll: the (persistent) flags resolve to A.
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == kCh4);                  // deferred while nextLines up
    d.Close();
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
    EventBus::Instance().Clear();
}
