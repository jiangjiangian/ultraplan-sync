#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "game/quest/Flags.h"
#include "game/state/InterludeExit.h"
#include "game/quest/ChapterGate.h"
#include "game/state/SemesterStateMachine.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterStateMachine;
using nccu::SemesterState;
using nccu::engine::math::Vec2;

/**
 * @file test_interlude_exit.cpp
 * @brief 驗證幕間市集南側「離開觸發區」的判定幾何、防卡關不變量，
 *        以及踏入觸發區後依旗標轉移回指定章節的流程。
 */

// 幕間的出口是地圖南側的觸發區，而非某個 NPC。以下測試固定其判定幾何與
// 防卡關不變量，並走一次與 GameController 每幀觸發相同的「旗標→轉移」路徑。

// 南側帶狀區內的點視為在區內，北邊的點視為在區外。
TEST_CASE("InterludeExit：南側帶狀區內的點視為在區內，北邊視為在區外") {
    // 深入南側帶狀區的點。
    CHECK(nccu::InInterludeExitZone(Vec2{500.0f, 2000.0f}));
    CHECK(nccu::InInterludeExitZone(Vec2{1800.0f, 1905.0f}));
    // 剛好在帶狀區北緣之上 -> 區外。
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{500.0f, 1899.0f}));
    // 更北邊（雨傘區，第一章交棒給幕間之處）。
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{500.0f, 1280.0f}));
    // 在 x 走廊範圍之外。
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{50.0f, 2000.0f}));
}

// 防卡關不變量：市集入口點不可落在出口區內。
TEST_CASE("InterludeExit：市集入口點不可落在出口區內（防卡關）") {
    // 若一進市集就已在出口帶狀區內，玩家會立刻被彈回去而完全略過市集。
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
}

// 踏入南側觸發區後，狀態機轉移到先前設定的 returnTo 章節。
TEST_CASE("InterludeExit：踏入南側觸發區後轉移到 returnTo 章節") {
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    // 依劇情主線：此次幕間結束後要回到第三章。
    m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    m.Transition(SemesterState::Interlude_Market);

    // 玩家在入口點 — 不在區內，不會離開。
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);

    // 玩家走進南側帶狀區：GameController 設下旗標，CheckChapterGates 消耗
    // 該旗標並轉移到 returnTo 章節。
    REQUIRE(nccu::InInterludeExitZone(Vec2{500.0f, 2000.0f}));
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));   // 旗標已被消耗
}
