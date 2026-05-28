#include "doctest/doctest.h"
#include "engine/platform/ScriptInput.h"
#include "controller/GameController.h"
#include "world/World.h"
#include "dialog/DialogSource.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

#include <sstream>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;
using nccu::gfx::Key;

// BUGLEDGER I4 regression lock.
//
// The real harness drives EVERY scripted run as, per frame:
//     script.Advance();                 // step classic timeline
//     script.ResolvePlan(snapshot);     // resolve high-level plan verbs
//     controller.Update();              // game reads input
// (see test_scriptinput_plan.cpp::RunPlan + Harness). Advance() runs
// BEFORE ResolvePlan() — so a classic `down D` directive is applied,
// then ResolvePlan() runs that same frame.
//
// A classic-only script has NO plan verbs => plan_ is empty. The
// Cycle-2 regression: ResolvePlan() unconditionally SynthUp'd W/A/S/D
// when `planPc_ >= plan_.size()` (always true with an empty plan),
// releasing the D key Advance() had just held — every frame. Net:
// classic WASD movement through the harness was dead. This test drives
// the exact Advance()/ResolvePlan() pair the loop uses and asserts the
// held key survives. Without the fix it reads released (regression).
TEST_CASE("ScriptInput: classic `down` survives ResolvePlan when plan is empty") {
    std::istringstream src(
        "# classic-only timeline: no plan verbs => plan_ is empty\n"
        "0 down D\n"
        "3 quit\n");
    ScriptInput in;
    in.Load(src);
    REQUIRE_FALSE(in.HasPlan());          // classic-only: no plan verbs

    // Frame 0: D goes down (Advance), then ResolvePlan must NOT release it.
    in.Advance();
    in.ResolvePlan(nullptr);              // mirror harness: null snapshot ok
    CHECK(in.IsDown(Key::D));             // held...
    CHECK(in.IsPressed(Key::D));          // ...with the press edge intact
    CHECK_FALSE(in.IsReleased(Key::D));   // and NOT spuriously released

    // Frame 1: still held across the Advance()/ResolvePlan() pair.
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsPressed(Key::D));    // no re-press edge while held
    CHECK_FALSE(in.IsReleased(Key::D));

    // Frame 2: still held — three full harness frames of motion survive.
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.IsDown(Key::D));
    CHECK_FALSE(in.IsReleased(Key::D));

    // Frame 3: classic `quit` fires; D still governed solely by classic
    // directives (never released by ResolvePlan, only by a `3 up D` we
    // didn't script — so it stays down here).
    in.Advance();
    in.ResolvePlan(nullptr);
    CHECK(in.WantsQuit());
    CHECK(in.IsDown(Key::D));
}

// Smoke: a minimal plan-verb script is unaffected by the fix. The I4
// early-return must short-circuit ONLY the plan-LESS path — a plan-
// bearing script must still flow through the resolver (incl. its
// `!world` idle and `planPc_ >= plan_.size()` exhausted-plan paths,
// which the fix left byte-identical). Driven through the SAME harness
// loop test_scriptinput_plan.cpp uses: per frame Advance() then
// ResolvePlan(prevSnapshot) with the snapshot lagging one frame.
TEST_CASE("ScriptInput: minimal plan verbs still resolve after the I4 fix") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    ScriptInput in;
    std::istringstream src(
        "wait 2\n"
        "quit\n");
    in.Load(src);
    nccu::gfx::Input::SetSource(&in);
    REQUIRE(in.HasPlan());                // plan-bearing: verbs present
    CHECK_FALSE(in.PlanDone());

    const World* snapshot = nullptr;      // null on frame 0, like harness
    bool quitObserved = false;
    for (int f = 0; f < 64 && !quitObserved; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;                // captured "at EndFrame"
        quitObserved = in.WantsQuit();
    }

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);

    // wait 2 then quit completed: the resolver ran end-to-end for a
    // plan-bearing script (proving the fix did not short-circuit it).
    CHECK(quitObserved);
    CHECK(in.WantsQuit());
    CHECK(in.PlanDone());                  // every verb completed
}
