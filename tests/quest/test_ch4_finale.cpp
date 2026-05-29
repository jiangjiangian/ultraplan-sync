/**
 * @file test_ch4_finale.cpp
 * @brief 驗證 Ch4 助教結算選單（體諒/質問/退出）的選項屬性、副作用，以及體諒走到 Ending A 的閘門延遲。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "game/state/EndingGate.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;

// 持續推進開場台詞，直到出現程式碼建構的結算選單。
void StepToChoice(nccu::DialogState& d) {
    int guard = 0;
    while (d.Active() && !d.AtChoice() && guard++ < 64) d.Advance();
}
}  // namespace

// 助教結算應呈現程式碼建構的「體諒／質問／退出」三選項；退出無任何副作用，體諒為 +15 並設 Ending A 鍵。
TEST_CASE("S5e-2d: Ch4 助教 結算 presents the code-constructed 體諒/質問 menu") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;

    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());
    StepToChoice(d);
    REQUIRE(d.AtChoice());
    // 選單尾端帶有一個不提交的退出項（索引 2）。體諒／質問維持索引 0/1
    //（退出固定附加在最後，沿用 vendor 拒買的契約），故以下路由／karma 斷言不變。
    REQUIRE(d.Choices().size() == 3);
    CHECK(d.Choices()[0].label == "體諒助教的辛勞");
    CHECK(d.Choices()[1].label == "質問／強硬索回");
    CHECK(d.Choices()[2].label == nccu::kDialogExitLabel);
    // 退出選項完全沒有任何副作用。
    CHECK(d.Choices()[2].karmaDelta == 0);
    CHECK(d.Choices()[2].setsFlag.empty());

    // 體諒（索引 0）：+15 karma，並設 Flag_ConsoledTA（Ending A 的鍵）。
    const nccu::DialogChoice* c = d.Advance();
    REQUIRE(c != nullptr);
    CHECK(c->label == "體諒助教的辛勞");
    CHECK(c->karmaDelta == 15);
    CHECK(c->setsFlag == nccu::kFlagConsoledTA);
    CHECK(c->flagValue == true);
}

// 質問分支是 -5 karma、不設旗標的路徑。
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

// 結算選擇已定案（Flag_TaFinaleChoiceMade）後，再對話只剩純台詞重述，不再出現選單。
TEST_CASE("S5e-2d: Flag_TaFinaleChoiceMade -> line-only recap (one-shot)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);          // 選擇已定案
    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());
    // 重述絕不會重新呈現選單——推進只會把台詞播完。
    int guard = 0;
    while (d.Active() && guard++ < 64) {
        CHECK_FALSE(d.AtChoice());
        d.Advance();
    }
    CHECK_FALSE(d.Active());
}

// 體諒選擇端到端走完 Ending A：閘門會延遲到收尾台詞關閉後才結算到 A。
TEST_CASE("S5e-2d: the 體諒 choice closes the Ending A path end-to-end") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    p.AddKarma(40);                                // 約 90，體諒前
    p.SetFlag(nccu::kFlagHasTrueUmbrella);             // 在 Ch4 重新取得真傘

    nccu::DialogState d;
    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    StepToChoice(d);
    REQUIRE(d.AtChoice());
    const nccu::DialogChoice* c = d.Advance();     // 體諒（索引 0）
    REQUIRE(c != nullptr);
    // 複製 GameController 的 ApplyDialogChoice（自由函式，未對外公開）。
    p.AddKarma(c->karmaDelta);
    if (!c->setsFlag.empty() && c->flagValue) p.SetFlag(c->setsFlag);
    CHECK(p.GetKarma() > 80);
    CHECK(p.HasFlag(nccu::kFlagConsoledTA));

    nccu::SemesterStateMachine m;
    m.Transition(kCh4);
    // 體諒選擇排入了收尾台詞，因此此時 d 仍處於 active；CheckEndingGates 會
    // 延遲到對話結束後才結算（讓玩家先讀完「拿回你的傘」這段）。先確認它延遲，
    // 再關閉對話框重新輪詢：持久旗標便結算為 A。
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == kCh4);                  // 收尾台詞還在時延遲
    d.Close();
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
    EventBus::Instance().Clear();
}
