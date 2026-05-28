#include "doctest/doctest.h"
#include "harness/ScriptInput.h"
#include "engine/input/Key.h"

#include <sstream>

using nccu::ScriptInput;
using nccu::gfx::Key;

// Locks the edge semantics the deterministic harness depends on: a single
// wrong edge silently desyncs every scripted playthrough.
TEST_CASE("ScriptInput: down/up hold and edge semantics mirror raylib") {
    std::istringstream src(
        "# comment line ignored\n"
        "0 down D\n"
        "3 up D\n");
    ScriptInput in;
    in.Load(src);

    in.Advance();                       // frame 0: D goes down
    CHECK(in.IsDown(Key::D));
    CHECK(in.IsPressed(Key::D));         // pressed edge only on frame 0
    CHECK_FALSE(in.IsReleased(Key::D));

    in.Advance();                       // frame 1: still held, no edge
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsPressed(Key::D));
    CHECK_FALSE(in.IsReleased(Key::D));

    in.Advance();                       // frame 2: still held
    CHECK(in.IsDown(Key::D));

    in.Advance();                       // frame 3: D released
    CHECK_FALSE(in.IsDown(Key::D));
    CHECK(in.IsReleased(Key::D));        // released edge only on frame 3
    CHECK_FALSE(in.IsPressed(Key::D));

    in.Advance();                       // frame 4: nothing lingers
    CHECK_FALSE(in.IsReleased(Key::D));
}

TEST_CASE("ScriptInput: press is a one-frame tap auto-released next frame") {
    std::istringstream src("5 press E\n");
    ScriptInput in;
    in.Load(src);

    for (int f = 0; f < 5; ++f) {
        in.Advance();
        CHECK_FALSE(in.IsDown(Key::E));
    }
    in.Advance();                       // frame 5: tap
    CHECK(in.IsDown(Key::E));
    CHECK(in.IsPressed(Key::E));

    in.Advance();                       // frame 6: auto-released
    CHECK_FALSE(in.IsDown(Key::E));
    CHECK(in.IsReleased(Key::E));
    CHECK_FALSE(in.IsPressed(Key::E));
}

TEST_CASE("ScriptInput: quit directive sets WantsQuit at its frame, not before") {
    std::istringstream src("2 quit\n");
    ScriptInput in;
    in.Load(src);

    in.Advance();  CHECK_FALSE(in.WantsQuit());   // frame 0
    in.Advance();  CHECK_FALSE(in.WantsQuit());   // frame 1
    in.Advance();  CHECK(in.WantsQuit());         // frame 2
}

TEST_CASE("ScriptInput: named keys and letters parse; junk lines are skipped") {
    std::istringstream src(
        "garbage that is not a directive\n"
        "0 down Space\n"
        "0 down Enter\n"
        "0 down ZZZ\n"          // unknown key token -> skipped
        "0 wiggle Left\n");     // unknown verb -> skipped
    ScriptInput in;
    in.Load(src);

    in.Advance();
    CHECK(in.IsDown(Key::Space));
    CHECK(in.IsDown(Key::Enter));
    CHECK_FALSE(in.IsDown(Key::Left));
}
