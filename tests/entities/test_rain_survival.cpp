// Regression guard for BUGLEDGER I8 — the GDD's core rain-survival loop.
//
// History: the meter was DEAD CODE (Player::ApplyRain had zero production
// callers). Cycle 3 wired it CONSERVATIVELY (accrue + observable, lethal
// teleport deferred). Cycle 4 makes it LIVE & LETHAL with the missing
// design piece — a DRAIN rule: GameController::Update() each frame either
//   * DRAINS  (Player::DrainRain, -10 u/s) when the player is SHELTERED
//     (under an umbrella OR inside a building), or
//   * ACCRUES (Player::ApplyRain lethal=true, +5 u/s) when EXPOSED
//     (outdoors and umbrella-less); at >=100 RespawnAtGate teleports the
//     player to the 正門 gate, resets the meter, emits a 落湯雞 message.
// (Skipped only in the market interlude and endings.) A full meter is now
// a manage-your-exposure failure, not a hidden one-shot timer — and with
// the drain the existing ending scripts stay winnable & deterministic.
//
// The GC cases drive the REAL GameController::Update() loop through the
// same gfx::Input choke point the harness uses (mirrors the seam in
// tests/test_i35_interact_vendor.cpp), exercising the production path.
//
// Revert-verify (each must FAIL without the Cycle-4 fix):
//   * Remove the GC rain tick entirely — the meter stays 0; the accrue
//     case's "rises" CHECK fails.
//   * Restore lethal=false (Cycle-3 conservative) — no teleport / no
//     落湯雞 message fires; the lethal case's soakMsg / reset CHECKs fail.
//   * Delete Player::DrainRain (or its sheltered branch in GC) — a
//     sheltered player's meter would not fall; the drain case fails.

#include "doctest/doctest.h"
#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "world/Buildings.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "state/SemesterState.h"
#include "engine/input/Input.h"
#include "engine/input/Key.h"
#include "engine/platform/Time.h"

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
// test_i35_interact_vendor.cpp). We never press a key — the player
// stands still and only the rain tick runs — but GameController still
// needs a live InputSource to poll.
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

// Unit — DrainRain is pure recovery: -10 u/s, clamped to [0,100], never
// teleports. Raise the meter first via ApplyRain(lethal=false) (umbrella-
// less accrual with the teleport suppressed), then drain it to the floor.
TEST_CASE("rain: DrainRain recovers -10 u/s and clamps at 0, no side effect") {
    Player p{Vec2{1234.0f, 5678.0f}};
    REQUIRE(p.GetRainMeter() == doctest::Approx(0.0f));

    // +5 u/s for 4 s => 20 (well clear of the cap and the floor).
    for (int i = 0; i < 240; ++i) p.ApplyRain(1.0f / 60.0f, /*lethal=*/false);
    const float raised = p.GetRainMeter();
    CHECK(raised == doctest::Approx(20.0f).epsilon(0.05));

    // -10 u/s for 1 s => -10 (raised ~20 -> ~10), strictly falling.
    float prev = raised;
    for (int i = 0; i < 60; ++i) {
        p.DrainRain(1.0f / 60.0f);
        const float now = p.GetRainMeter();
        CHECK(now < prev);
        prev = now;
    }
    CHECK(p.GetRainMeter() == doctest::Approx(raised - 10.0f).epsilon(0.05));

    // Drains clamp at 0 — never negative, and the position is untouched
    // (DrainRain must not teleport like the lethal accrual does).
    for (int i = 0; i < 600; ++i) p.DrainRain(1.0f / 60.0f);
    CHECK(p.GetRainMeter() == doctest::Approx(0.0f));
    CHECK(p.GetPosition().x == doctest::Approx(1234.0f));
    CHECK(p.GetPosition().y == doctest::Approx(5678.0f));
}

// REQUIREMENT #5 unit — ApplyRainSheltered (umbrella-but-outdoors) is a
// SLOW accrual: +1.5 u/s ≈ 30 % of the exposed +5 u/s, clamped [0,100],
// lethal-armed (teleports + resets at the cap, like ApplyRain). It must
// be far slower than ApplyRain over the same time and (unlike ApplyRain)
// NOT honour the umbrella flag (this IS the umbrella case). Reverting
// the GC's umbrella-outdoors arm to DrainRain doesn't touch this unit;
// removing/zeroing ApplyRainSheltered's accrual fails it.
TEST_CASE("REQ#5 unit: ApplyRainSheltered slow-accrues (~1.5 u/s), lethal-armed") {
    Player p{Vec2{100.0f, 200.0f}};
    p.SetHasUmbrella(true);                  // it must accrue ANYWAY
    REQUIRE(p.GetRainMeter() == doctest::Approx(0.0f));

    // 4 s sheltered-outdoors => ~6 (1.5 u/s), strictly rising.
    float prev = 0.0f;
    for (int i = 0; i < 240; ++i) {
        p.ApplyRainSheltered(1.0f / 60.0f, /*lethal=*/false);
        const float now = p.GetRainMeter();
        CHECK(now >= prev);
        prev = now;
    }
    CHECK(p.GetRainMeter() == doctest::Approx(6.0f).epsilon(0.05));

    // Much slower than exposed: a fresh player accruing ApplyRain for the
    // SAME 4 s reaches ~20 (>3x). Slow ≠ zero, slow ≠ fast.
    Player q{Vec2{0.0f, 0.0f}};
    for (int i = 0; i < 240; ++i) q.ApplyRain(1.0f / 60.0f, /*lethal=*/false);
    CHECK(q.GetRainMeter() > p.GetRainMeter() * 2.5f);

    // Lethal-armed: drive it past the cap with the teleport ON — the
    // meter resets (not pinned at 100) and the player is moved to 正門.
    Player r{Vec2{1500.0f, 1500.0f}};
    r.SetHasUmbrella(true);
    for (int i = 0; i < 60 * 80; ++i)        // 80 s * 1.5 ≈ 120 >> 100
        r.ApplyRainSheltered(1.0f / 60.0f, /*lethal=*/true);
    CHECK(r.GetRainMeter() < 100.0f);        // reset, not stuck at the cap
    CHECK(r.GetPosition().x == doctest::Approx(500.0f));   // 正門 respawn
    CHECK(r.GetPosition().y == doctest::Approx(1860.0f));
}

// Cycle-4 headline (inverts the old conservative case) — an umbrella-less
// player standing OUTDOORS (Ch1 spawn is south of every building rect, so
// CurrentBuildingName() is empty) accrues +5 u/s through the REAL GC tick
// and, on saturation, the LETHAL teleport fires: a 落湯雞 ShowMessage is
// emitted and the meter is reset (it does NOT pin at 100).
TEST_CASE("rain: outdoor umbrella-less player accrues, then the lethal gate fires") {
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
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);
    REQUIRE_FALSE(p->HasUmbrella());
    REQUIRE(p->GetRainMeter() == doctest::Approx(0.0f));

    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    Frame(controller, in);
    CHECK(world.CurrentBuildingName().empty());      // confirmed outdoors
    CHECK(p->GetRainMeter() > 0.0f);                  // accruing (not drain)

    // Rise toward the cap: strictly increasing while below 100 and before
    // the first teleport (the meter sawtooths once RespawnAtGate fires).
    float prev = p->GetRainMeter();
    bool sawHighThenDrop = false;
    for (int f = 0; f < 60 && soakMsgHits == 0; ++f) {
        Frame(controller, in);
        const float now = p->GetRainMeter();
        if (now < 100.0f && soakMsgHits == 0) CHECK(now >= prev);
        prev = now;
    }

    // Drive well past saturation (100/5 = 20 s; 1800 frames = 30 s). The
    // lethal gate must have fired at least once, and each firing resets
    // the meter (so it cannot stay pinned at 100 — it sawtooths down).
    float maxSeen = 0.0f;
    for (int f = 0; f < 1800; ++f) {
        const float before = p->GetRainMeter();
        Frame(controller, in);
        const float after = p->GetRainMeter();
        if (before > maxSeen) maxSeen = before;
        if (before > 80.0f && after < 20.0f) sawHighThenDrop = true; // reset
    }
    CHECK(soakMsgHits >= 1);            // 落湯雞 ShowMessage (only from gate)
    CHECK(maxSeen >= 99.0f);            // it really did reach the cap
    CHECK(sawHighThenDrop);            // and was RESET (lethal, not pinned)
    CHECK(p->GetRainMeter() < 100.0f); // not stuck at the cap

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// REQUIREMENT #5 (replaces the old Cycle-4 "umbrella => GC drains" case;
// post-design-change rewrite — the I6/F2 precedent). Rain pressure must
// exist EVERY chapter, so an umbrella no longer grants rain immunity:
// the GC tick is now 3-way and this pins all three arms via the REAL
// loop.
//   * OUTDOORS, NO UMBRELLA  -> accrues fast (+5 u/s, ApplyRain)
//   * OUTDOORS, WITH UMBRELLA-> still accrues, but SLOWLY (+1.5 u/s,
//                               ApplyRainSheltered) — an umbrella buys
//                               time, it is NOT immunity (the #5 fix)
//   * INSIDE A BUILDING      -> DRAINS to 0 (-10 u/s, the true haven)
// Revert-verify: restoring the old `sheltered = HasUmbrella() || in
// building => DrainRain` 2-way tick makes the "umbrella outdoors still
// accrues" CHECK fail (it would drain instead).
TEST_CASE("REQ#5: umbrella SLOWS rain (every chapter); only a building dries you") {
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
    REQUIRE(world.Semester().Current() == SemesterState::Chapter1_AddDrop);

    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    // (1) ~3 s OUTDOORS, no umbrella => fast accrual (+5 u/s ⇒ ~15).
    for (int f = 0; f < 180; ++f) Frame(controller, in);
    const float wet = p->GetRainMeter();
    CHECK(wet > 12.0f);                       // ~15 at +5 u/s over 3 s
    CHECK(soakMsgHits == 0);

    // (2) Equip an umbrella but STAY OUTDOORS (Ch1 spawn is south of
    //     every building rect ⇒ CurrentBuildingName() empty). The meter
    //     must KEEP RISING — slowly (#5: an umbrella is not immunity) —
    //     strictly upward, far slower than the exposed rate.
    p->SetHasUmbrella(true);
    float prev = p->GetRainMeter();
    for (int f = 0; f < 120; ++f) {           // 2 s sheltered-outdoors
        Frame(controller, in);
        const float now = p->GetRainMeter();
        CHECK(now >= prev);                   // still accruing, not draining
        prev = now;
    }
    const float slowGain = p->GetRainMeter() - wet;
    CHECK(slowGain > 0.0f);                   // it DID rise (no immunity)
    // Over 2 s: umbrella ≈ +1.5 u/s ⇒ ~3; exposed would have been +5 u/s
    // ⇒ ~10. Assert it is in the slow band (well under the fast rate).
    CHECK(slowGain < 5.0f);
    CHECK(soakMsgHits == 0);

    // (3) Step INSIDE a building (its trigger rect centre) — only here
    //     does the meter DRAIN, strictly downward, to 0, no teleport.
    const auto& b = nccu::buildings::kAll[0];   // 大勇樓
    p->SetPosition(nccu::gfx::Vec2{
        b.triggerRect.x + b.triggerRect.width  * 0.5f,
        b.triggerRect.y + b.triggerRect.height * 0.5f});
    Frame(controller, in);                      // BuildingTracker latches
    float dprev = p->GetRainMeter();
    for (int f = 0; f < 60; ++f) {
        Frame(controller, in);
        const float now = p->GetRainMeter();
        CHECK(now <= dprev);                    // draining indoors
        dprev = now;
    }
    CHECK(p->GetRainMeter() < slowGain + wet);  // recovered
    for (int f = 0; f < 600; ++f) Frame(controller, in);
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));   // fully dried
    CHECK(soakMsgHits == 0);                              // never teleported

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// Rain is gameplay-only — the market interlude is a safe state; GC skips
// the tick entirely, so an umbrella-less player's meter stays put.
TEST_CASE("rain: no accrual or drain in the Interlude_Market (safe state)") {
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
    CHECK(p->GetRainMeter() == doctest::Approx(0.0f));  // market => no tick

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
