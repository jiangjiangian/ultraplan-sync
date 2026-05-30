/**
 * @file test_ch4_gentle_umbrella.cpp
 * @brief 驗證溫柔結局會交回真傘：體諒+karma>80→Ending A；體諒但低 karma→Ending D；質問→Ending B。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter4Quest.h"
#include "engine/events/EventBus.h"
#include "game/state/EndingGate.h"
#include "game/state/SemesterStateMachine.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterState;

// 溫柔結局（體諒，Flag_ConsoledTA）也會把玩家的真傘交回——敘事上，當你善待
// 助教時，他會把你的傘塞回你手裡。因此 體諒 → Flag_ConsoledTA +
// Flag_HasTrueUmbrella →（在 karma>80 時）Ending A，且不需要 Ch4 的隱藏傘。
// 強硬的質問路徑不會設 Flag_ConsoledTA，因此拿不到傘，仍導向 Ending B（冷淡結局）。
// A 結局的 karma>80 閘門維持不變。TryGrantTaFinaleUmbrella 是任務層的輔助函式，
// 由 GameController 在確認助教結局選擇時呼叫。

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

// 選擇體諒（Flag_ConsoledTA）後，輔助函式應同時授予 Flag_HasTrueUmbrella 與實際持傘，且具冪等性。
TEST_CASE("體諒（Flag_ConsoledTA）授予 Flag_HasTrueUmbrella 與 HasUmbrella") {
    Player p = MakePlayer();
    p.SetFlag(nccu::kFlagConsoledTA);                 // 選了體諒
    REQUIRE_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    REQUIRE_FALSE(p.HasUmbrella());

    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));     // Ending A 的持傘條件
    CHECK(p.HasUmbrella());                       // 實際持有（會擋雨）

    // 重複對話／重複確認時具冪等性。
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));
}

// 強硬或尚未做的結局都拿不到傘：質問路徑與對象／章節不符時，溫柔授予一律無操作。
TEST_CASE("強硬或尚未做的結局都拿不到傘") {
    SUBCASE("無體諒旗標 -> 不授予（強硬質問路徑）") {
        Player p = MakePlayer();
        // 質問路徑：GameController 會設 Flag_TaFinaleChoiceMade，但
        // 不會設 Flag_ConsoledTA——因此溫柔授予必須無操作。
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
        CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
        CHECK_FALSE(p.HasUmbrella());
    }
    SUBCASE("對象不符／章節不符 -> 即使有體諒也無操作") {
        Player p = MakePlayer();
        p.SetFlag(nccu::kFlagConsoledTA);
        nccu::TryGrantTaFinaleUmbrella(p, "victim", kCh4);          // 對象不符
        nccu::TryGrantTaFinaleUmbrella(
            p, "ta", SemesterState::Chapter1_AddDrop);              // 非 Ch4
        CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    }
}

// 體諒 + karma>80 不需隱藏傘即可抵達 Ending A：依 GameController 的確認順序端到端驗證。
TEST_CASE("體諒 + karma>80 不需隱藏傘即可抵達 Ending A") {
    // 完整走一遍溫柔路徑，依 GameController 確認時的實際順序：ApplyDialogChoice
    // 設 Flag_ConsoledTA（+15）、設 Flag_TaFinaleChoiceMade，接著
    // TryGrantTaFinaleUmbrella 授予持傘旗標，最後才跑 CheckEndingGates。
    // 授予發生在閘門之前，因此光憑 karma>80 + 體諒 就能抵達 Ending A，
    // 不需要另外撿 Ch4 的隱藏傘。
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.AddKarma(100);                              // > 80
    p.SetFlag(nccu::kFlagConsoledTA);                 // 選了體諒（ApplyDialogChoice）
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);         // GameController 的自鎖旗標
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // 溫柔授予
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

// 體諒但 karma<=80 的玩家仍會拿到傘，並落入苦樂參半的 Ending D，不會卡在 Ch4。
TEST_CASE("體諒但 karma<=80 的玩家仍拿到傘（-> Ending D，不卡關）") {
    // 溫柔分支的授予是無條件的（不受 karma 閘門限制）；只有 Ending A 的
    // karma>80 閘門決定走 A 還是其他結局。體諒但未達 karma>80 的玩家會落入
    // 苦樂參半的 Ending D（風雨同行），絕不會卡在 Ch4。
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();                      // karma 約 50
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagConsoledTA);
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag(nccu::kFlagHasTrueUmbrella));     // 拿到了傘
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_D);   // karma<=80 -> D
}

// 強硬質問（冷淡結局）即使高 karma 仍導向 Ending B 而非 A（沒有體諒就拿不到傘）。
TEST_CASE("強硬質問（冷淡結局）仍導向 Ending B 而非 A") {
    // 強硬分支：高 karma 但沒有體諒。冷淡結局 =
    // Flag_TaFinaleChoiceMade && !Flag_ConsoledTA -> Ending B。
    // 溫柔授予因缺 ConsoledTA 而無操作，沒有傘，A 不可能達成。
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.AddKarma(100);
    p.SetFlag(nccu::kFlagTaFinaleChoiceMade);         // 已確認質問
    // GameController 會呼叫授予；缺 ConsoledTA 時為無操作。
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));
    nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Ending_B);   // 冷淡結局勝出
}

// 溫柔傘的改動不影響既有的 Ending B（詛咒傘）／Ending C（醜傘）路徑。
TEST_CASE("溫柔傘的改動不影響既有的 Ending B/C") {
    SUBCASE("詛咒傘 -> B，與新輔助函式無關") {
        nccu::SemesterStateMachine m; m.Transition(kCh4);
        Player p = MakePlayer(); nccu::DialogState d;
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // 無 ConsoledTA -> 無操作
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
    SUBCASE("買過醜傘 -> C，與新輔助函式無關") {
        nccu::SemesterStateMachine m; m.Transition(kCh4);
        Player p = MakePlayer(); nccu::DialogState d;
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // 無操作
        nccu::CheckEndingGates(EventBus::Instance(), p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);
    }
}
