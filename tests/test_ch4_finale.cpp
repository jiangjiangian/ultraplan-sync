#include "doctest/doctest.h"
#include "DialogOpener.h"
#include "DialogState.h"
#include "DialogSource.h"
#include "EndingGate.h"
#include "EventBus.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "gfx/Vec2.h"

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
    REQUIRE(d.Choices().size() == 2);
    CHECK(d.Choices()[0].label == "體諒助教的辛勞");
    CHECK(d.Choices()[1].label == "質問／強硬索回");

    // 體諒 (index 0): +15 karma, sets Flag_ConsoledTA (Ending A key).
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "體諒助教的辛勞");
    CHECK(c->karmaDelta == 15);
    CHECK(c->setsFlag == "Flag_ConsoledTA");
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
    p.SetFlag("Flag_TaFinaleChoiceMade");          // choice already made
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
    p.SetFlag("Flag_HasTrueUmbrella");             // re-claimed Ch4 True

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
    CHECK(p.HasFlag("Flag_ConsoledTA"));

    nccu::SemesterStateMachine m;
    m.Transition(kCh4);
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
    EventBus::Instance().Clear();
}
