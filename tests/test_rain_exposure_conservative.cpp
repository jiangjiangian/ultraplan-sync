// Regression guard for BUGLEDGER I8 — the GDD's core rain-survival meter
// was DEAD CODE: Player::ApplyRain had ZERO production callers, so
// GetRainMeter() was 0 for the entire game. Cycle 3 wires it
// CONSERVATIVELY in GameController::Update(): the meter genuinely
// accumulates OUTDOORS and is observable, but the lethal ≥100→正門
// teleport (and any movement-speed penalty) stays DEFERRED so it cannot
// derail the deterministic goto-based ending scripts.
//
// These cases drive the REAL GameController::Update() loop through the
// same gfx::Input choke point the harness uses (mirrors the seam in
// tests/test_i35_interact_vendor.cpp), exercising the exact production
// path rather than calling Player::ApplyRain directly.
//
// Revert-verify (each must FAIL without the production fix):
//   * Remove the `player->ApplyRain(dt, /*lethal=*/false);` rain tick in
//     GameController::Update() — the meter stays 0 forever, so the
//     "strictly rises then clamps" case fails at the first CHECK.
//   * Re-enable the teleport at the engine seam (call ApplyRain with
//     lethal=true / restore the unconditional RespawnAtGate) — the
//     "no teleport / position unchanged / no 落湯雞 message / meter not
//     reset" assertions fail (the player would be flung to {500,1860}
//     and the meter zeroed once it hit 100).

#include "doctest/doctest.h"
#include "GameController.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "DialogSource.h"
#include "EventBus.h"
#include "SemesterState.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"

#include <set>
#include <string>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::gfx::Key;
using nccu::gfx::Vec2;

namespace {

// Minimal scripted InputSource with raylib edge semantics, driven the
// exact way the harness drives ScriptInput (identical to the seam in
// test_i35_interact_vendor.cpp). We never press a key in these cases —
// the player stands still and only the rain tick runs — but
// GameController still needs a live InputSource to poll.
class TestInput final : public nccu::gfx::InputSource {
public:
    void EndFrame() { pressed_.clear(); released_.clear(); }
    bool IsDown(Key)     const noexcept override { return false; }
    bool IsPressed(Key)  const noexcept override { return false; }
    bool IsReleased(Key) const noexcept override { return false; }
private:
    std::set<int> pressed_, released_;
};

void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

}  // namespace

// I8 headline — an umbrella-less player standing OUTDOORS (the default
// Ch1 spawn {500,1860} is south of every building trigger rect, so
// CurrentBuildingName() is empty and the semester is Chapter1_AddDrop)
// accumulates rain every frame. The meter must strictly rise frame over
// frame and then CLAMP at exactly 100 — and crucially NO lethal teleport
// fires: the position is unchanged, no "落湯雞" ShowMessage is emitted,
// and the meter is NOT reset back toward 0 once it reaches 100.
TEST_CASE("I8: outdoor umbrella-less player accrues rain, clamps at 100, no teleport") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int soakMsgHits = 0;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) {
            if (e.text.find("落湯雞") != std::string::npos) ++soakMsgHits;
        });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);

    // Default state precondition: Ch1, no umbrella, outdoors, meter 0.
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);
    REQUIRE_FALSE(p->HasUmbrella());
    REQUIRE(p->GetRainMeter() == doctest::Approx(0.0f));

    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    const Vec2 spawn = p->GetPosition();

    // Frame 0: tracker has not run yet, meter still 0. Tick a few frames;
    // the meter must STRICTLY rise (5 u/s * 1/60 ≈ 0.0833 per frame).
    Frame(controller, in);
    const float afterOne = p->GetRainMeter();
    CHECK(afterOne > 0.0f);
    REQUIRE_FALSE(world.Dialog().Active());
    CHECK(world.CurrentBuildingName().empty());  // confirmed outdoors

    float prev = afterOne;
    for (int f = 0; f < 30; ++f) {
        Frame(controller, in);
        const float now = p->GetRainMeter();
        CHECK(now >= prev);                 // monotone non-decreasing
        if (now < 100.0f) CHECK(now > prev); // strictly rising below cap
        prev = now;
    }

    // Drive long past saturation: 100 / 5 = 20 s of exposure; 60*25 =
    // 1500 frames is comfortably over. Meter must pin at exactly 100.
    for (int f = 0; f < 1500; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(100.0f));

    // The conservative-wire invariant: NO lethal teleport this cycle.
    const Vec2 end = p->GetPosition();
    CHECK(end.x == doctest::Approx(spawn.x));   // never flung to the gate
    CHECK(end.y == doctest::Approx(spawn.y));
    CHECK(soakMsgHits == 0);                     // no 落湯雞 ShowMessage
    CHECK(p->GetRainMeter() == doctest::Approx(100.0f)); // meter NOT reset

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I8 companion A — an umbrella nullifies exposure. ApplyRain self-
// suppresses on hasUmbrella_, so even outdoors the meter stays 0. (This
// also documents that the GameController tick does not duplicate the
// umbrella check — it relies on ApplyRain's own guard.)
TEST_CASE("I8: an umbrella-equipped outdoor player accrues no rain") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);
    p->SetHasUmbrella(true);

    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    for (int f = 0; f < 600; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));  // umbrella => dry

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I8 companion B — rain is an OUTDOOR mechanic only. In the market
// interlude (Interlude_Market) the player is "indoors" by design intent,
// so the GameController tick must NOT call ApplyRain and the meter stays
// 0 even without an umbrella.
TEST_CASE("I8: no rain accrues in the Interlude_Market (indoors by design)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};
    world.Semester().Transition(SemesterState::Interlude_Market);

    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                       // roster -> Interlude
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE_FALSE(p->HasUmbrella());
    REQUIRE(world.Semester().Current() == SemesterState::Interlude_Market);

    for (int f = 0; f < 600; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));  // market => no rain

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
