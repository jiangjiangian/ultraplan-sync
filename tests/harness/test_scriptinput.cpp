#include "doctest/doctest.h"
#include "engine/platform/ScriptInput.h"
#include "engine/input/Key.h"

#include <sstream>

/**
 * @file test_scriptinput.cpp
 * @brief 驗證確定性輸入驅動器 ScriptInput 的解析與 edge 語意：down/up 的按住與邊緣須與
 *        raylib 一致、press 為單格 tap 下一格自動放開、quit 在其指定格才生效，以及具名鍵／
 *        字母可解析而垃圾行被略過。任一個錯誤的 edge 都會悄悄讓每一場腳本化執行失準。
 */

using nccu::ScriptInput;
using nccu::engine::input::Key;

// down/up 的按住與 edge 語意對應 raylib。
TEST_CASE("ScriptInput：down/up 的按住與 edge 語意對應 raylib") {
    std::istringstream src(
        "# comment line ignored\n"
        "0 down D\n"
        "3 up D\n");
    ScriptInput in;
    in.Load(src);

    in.Advance();                       // 第 0 格：D 按下
    CHECK(in.IsDown(Key::D));
    CHECK(in.IsPressed(Key::D));         // pressed edge 只在第 0 格
    CHECK_FALSE(in.IsReleased(Key::D));

    in.Advance();                       // 第 1 格：仍按住，無 edge
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsPressed(Key::D));
    CHECK_FALSE(in.IsReleased(Key::D));

    in.Advance();                       // 第 2 格：仍按住
    CHECK(in.IsDown(Key::D));

    in.Advance();                       // 第 3 格：D 放開
    CHECK_FALSE(in.IsDown(Key::D));
    CHECK(in.IsReleased(Key::D));        // released edge 只在第 3 格
    CHECK_FALSE(in.IsPressed(Key::D));

    in.Advance();                       // 第 4 格：不留殘餘
    CHECK_FALSE(in.IsReleased(Key::D));
}

// press 是單格 tap，下一格自動放開。
TEST_CASE("ScriptInput：press 是單格 tap，下一格自動放開") {
    std::istringstream src("5 press E\n");
    ScriptInput in;
    in.Load(src);

    for (int f = 0; f < 5; ++f) {
        in.Advance();
        CHECK_FALSE(in.IsDown(Key::E));
    }
    in.Advance();                       // 第 5 格：tap
    CHECK(in.IsDown(Key::E));
    CHECK(in.IsPressed(Key::E));

    in.Advance();                       // 第 6 格：自動放開
    CHECK_FALSE(in.IsDown(Key::E));
    CHECK(in.IsReleased(Key::E));
    CHECK_FALSE(in.IsPressed(Key::E));
}

// quit 指令在其指定格才設定 WantsQuit，之前不會。
TEST_CASE("ScriptInput：quit 指令在其指定格才設定 WantsQuit，之前不會") {
    std::istringstream src("2 quit\n");
    ScriptInput in;
    in.Load(src);

    in.Advance();  CHECK_FALSE(in.WantsQuit());   // 第 0 格
    in.Advance();  CHECK_FALSE(in.WantsQuit());   // 第 1 格
    in.Advance();  CHECK(in.WantsQuit());         // 第 2 格
}

// 具名鍵與字母可解析；垃圾行被略過。
TEST_CASE("ScriptInput：具名鍵與字母可解析，垃圾行被略過") {
    std::istringstream src(
        "garbage that is not a directive\n"
        "0 down Space\n"
        "0 down Enter\n"
        "0 down ZZZ\n"          // 未知的鍵 token -> 略過
        "0 wiggle Left\n");     // 未知的動詞 -> 略過
    ScriptInput in;
    in.Load(src);

    in.Advance();
    CHECK(in.IsDown(Key::Space));
    CHECK(in.IsDown(Key::Enter));
    CHECK_FALSE(in.IsDown(Key::Left));
}
