#include "doctest/doctest.h"
#include "engine/platform/ScriptInput.h"
#include "game/controller/GameController.h"
#include "engine/events/EventBus.h"
#include "game/world/World.h"
#include "game/dialog/DialogSource.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <sstream>

/**
 * @file test_scriptinput_classic_move.cpp
 * @brief 驗證 classic 計時指令（`down`/`up`）在每格 Advance() 之後緊接 ResolvePlan() 時仍能
 *        正常運作：當沒有任何計畫動詞（plan 為空）時，ResolvePlan() 不得釋放 Advance() 剛按住
 *        的鍵；同時確認帶有計畫動詞的腳本仍會正常流經解析器。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;
using nccu::engine::input::Key;

// 真正的 harness 每格依序執行：
//     script.Advance();                 // 推進 classic 時間軸
//     script.ResolvePlan(snapshot);     // 解析高階計畫動詞
//     controller.Update();              // 遊戲讀取輸入
//（見 test_scriptinput_plan.cpp::RunPlan + Harness）。Advance() 在 ResolvePlan() 之前——故
// classic 的 `down D` 指令先套用，ResolvePlan() 接著在同一格執行。
//
// 純 classic 腳本沒有計畫動詞 => plan_ 為空。此處守護的迴歸是：當 plan 為空時，ResolvePlan()
// 不得每格都把 W/A/S/D SynthUp（這會釋放 Advance() 剛按住的鍵），否則經由 harness 的 classic
// WASD 移動會失效。本測試驅動迴圈所用的同一個 Advance()/ResolvePlan() 配對，並斷言按住的鍵
// 存活。
TEST_CASE("ScriptInput：plan 為空時 classic 的 `down` 能在 ResolvePlan 後存活") {
    std::istringstream src(
        "# classic-only timeline: no plan verbs => plan_ is empty\n"
        "0 down D\n"
        "3 quit\n");
    ScriptInput in;
    in.Load(src);
    REQUIRE_FALSE(in.HasPlan());          // 純 classic：無計畫動詞

    // 第 0 格：D 按下（Advance），接著 ResolvePlan 不得釋放它。
    in.Advance();
    in.ResolvePlan(nullptr);              // 對應 harness：null 快照可接受
    CHECK(in.IsDown(Key::D));             // 按住……
    CHECK(in.IsPressed(Key::D));          // ……且 press edge 完好
    CHECK_FALSE(in.IsReleased(Key::D));   // 且未被錯誤釋放

    // 第 1 格：跨過 Advance()/ResolvePlan() 配對仍按住。
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsPressed(Key::D));    // 按住期間不再有 press edge
    CHECK_FALSE(in.IsReleased(Key::D));

    // 第 2 格：仍按住——連三整格的 harness 移動皆存活。
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsReleased(Key::D));

    // 第 3 格：classic `quit` 觸發；D 仍僅由 classic 指令掌控（從未被 ResolvePlan 釋放，且
    // 我們沒有腳本化 `3 up D`，故此處仍為按住）。
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.WantsQuit());
    CHECK(in.IsDown(Key::D));
}

// 冒煙測試：最小的計畫動詞腳本不受此修正影響。提前返回只能短路「無計畫」路徑——帶計畫的腳本
// 仍須流經解析器（含其 `!world` 閒置與 `planPc_ >= plan_.size()` 計畫耗盡等路徑，修正未動到
// 它們）。驅動方式與 test_scriptinput_plan.cpp 所用的 harness 迴圈相同：每格 Advance() 後
// ResolvePlan(prevSnapshot)，快照落後一格。
TEST_CASE("ScriptInput：最小的計畫動詞腳本仍能正常解析") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    ScriptInput in;
    std::istringstream src(
        "wait 2\n"
        "quit\n");
    in.Load(src);
    nccu::engine::input::Input::SetSource(&in);
    REQUIRE(in.HasPlan());                // 帶計畫：有動詞
    CHECK_FALSE(in.PlanDone());

    const World* snapshot = nullptr;      // 第 0 格為 null，與 harness 相同
    bool quitObserved = false;
    for (int f = 0; f < 64 && !quitObserved; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;                // 於 EndFrame 擷取
        quitObserved = in.WantsQuit();
    }

    nccu::engine::input::Input::SetSource(nullptr);
    nccu::engine::platform::Time::SetFixedStep(0.0f);

    // wait 2 後 quit 完成：解析器對帶計畫的腳本端到端執行（證明修正未把它短路）。
    CHECK(quitObserved);
    CHECK(in.WantsQuit());
    CHECK(in.PlanDone());                  // 每個動詞皆完成
}
