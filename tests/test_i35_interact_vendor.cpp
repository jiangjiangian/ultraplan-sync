// Regression guards for BUGLEDGER I3 (interact-an-NPC is geometrically
// impossible — the whole A/B/C spine soft-locked in Ch1) and I5
// (Vendor::TryBuy had no runtime caller — Ending C / the Ch2 EnergyDrink
// were unreachable). Both drive the REAL GameController::Update() loop
// through the same gfx::Input choke point the harness uses, so they
// exercise the exact production path, not a unit-level shim.
//
// Revert-verify (both must FAIL without the production fix):
//   * I3: restore the E-probe to `Rect{px,py,24,24}` (no kInteractReach
//     inflation) — "walk up + E opens the victim dialog" fails: the
//     flush-blocked player's probe never overlaps the NPC hitbox.
//   * I5: drop the `o.IsVendor()` branch in the E-interact lambda (let a
//     Vendor fall to o.Interact()) — the buy menu never opens, TryBuy is
//     never called, Flag_BoughtUglyUmbrella / Flag_Ch2Cleared stay unset.

#include "doctest/doctest.h"
#include "GameController.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "DialogSource.h"
#include "ChapterVendors.h"
#include "EventBus.h"
#include "GameObject.h"
#include "SemesterState.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"

#include <set>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::World;
using nccu::SemesterState;
using nccu::gfx::Key;

namespace {

// Minimal scripted InputSource with raylib edge semantics, driven the
// EXACT way the harness drives ScriptInput: each frame is
//   set this frame's keys; controller.Update(); src.EndFrame();
// so a Tap()'d key is "pressed" for precisely the one Update() that
// follows it and auto-releases at EndFrame() — identical edge contract
// to LiveInput/ScriptInput, so GameController cannot tell the difference
// (we never touch ScriptInput.* — out of scope).
class TestInput final : public nccu::gfx::InputSource {
public:
    void Hold(Key k)    { if (down_.insert(static_cast<int>(k)).second) pressed_.insert(static_cast<int>(k)); }
    void Release(Key k) { if (down_.erase(static_cast<int>(k)))         released_.insert(static_cast<int>(k)); }
    void Tap(Key k)     { Hold(k); autoUp_.insert(static_cast<int>(k)); }

    // Called AFTER controller.Update(): expire the one-frame press/release
    // edges and auto-up any tapped key (so the press was exactly one
    // Update wide). Held keys (Hold without Tap) persist across frames.
    void EndFrame() {
        pressed_.clear();
        released_.clear();
        for (int k : autoUp_) { if (down_.erase(k)) released_.insert(k); }
        autoUp_.clear();
    }

    bool IsDown(Key k)     const noexcept override { return down_.count(static_cast<int>(k)) != 0; }
    bool IsPressed(Key k)  const noexcept override { return pressed_.count(static_cast<int>(k)) != 0; }
    bool IsReleased(Key k) const noexcept override { return released_.count(static_cast<int>(k)) != 0; }

private:
    std::set<int> down_, pressed_, released_, autoUp_;
};

// One simulation frame: keys already set for this frame -> Update ->
// expire the per-frame edges (mirror Harness::EndFrame).
void Frame(nccu::GameController& c, TestInput& in) {
    c.Update();
    in.EndFrame();
}

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

const GameObject* FindVendor(const World& w) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->IsVendor()) return u.get();
    return nullptr;
}

// Walk the player to flush-stop, then tap E once per frame until a dialog
// opens (or budget exhausts). Returns whether a dialog opened.
bool WalkUpAndTalk(nccu::GameController& c, TestInput& in, World& w,
                   Key approach, float wantX) {
    in.Hold(approach);
    Player* p = w.GetPlayer();
    float lastX = p->GetPosition().x, lastY = p->GetPosition().y;
    for (int f = 0; f < 800; ++f) {
        Frame(c, in);
        const auto pos = w.GetPlayer()->GetPosition();
        if (f > 5 && pos.x == lastX && pos.y == lastY) break;  // flush
        lastX = pos.x; lastY = pos.y;
    }
    in.Release(approach);
    (void)wantX;
    for (int f = 0; f < 16; ++f) {
        in.Tap(Key::E);
        Frame(c, in);
        if (w.Dialog().Active()) return true;
    }
    return w.Dialog().Active();
}

}  // namespace

// I3 — the headline lock. A player WALKS (held Key::A) up to the Ch1 苦主
// the way a human or the harness would, gets flush-blocked by the NPC's
// movement collider, taps E, and the conversation must open. Before the
// fix the E-probe equals the movement box so a flush stop never overlaps
// the NPC and dialog NEVER opens (the entire spine soft-locks here).
TEST_CASE("I3: walking up to the Ch1 victim + E opens the dialog") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    const GameObject* v = FindNpc(world, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;             // {380,1860}

    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    const bool opened = WalkUpAndTalk(controller, in, world, Key::A, vx);

    // Flush against the victim: pre-fix this is where the spine dies. The
    // player can be touched-but-not-overlapped, so a 24x24 E-probe at the
    // player origin never collides. The interaction reach fix makes the
    // inflated probe overlap the NPC hitbox.
    const float px = world.GetPlayer()->GetPosition().x;
    CHECK(px >= vx);                                   // never walked through
    CHECK(px <= vx + 40.0f);                            // truly adjacent
    CHECK(opened);                                      // <-- the I3 lock
    CHECK(world.Dialog().Active());
    CHECK(world.Dialog().NpcId() == "victim");

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I3 companion — the fix must NOT open a walk-through hole. The player
// presses INTO the victim for many frames; the collider must keep the
// player's box from ever strictly overlapping the NPC's box (flush is OK,
// pass-through is not).
TEST_CASE("I3: player still cannot walk through a static NPC") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};
    const GameObject* v = FindNpc(world, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;
    const float vy = v->GetPosition().y;

    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    in.Hold(Key::A);                                   // shove left, hard
    for (int f = 0; f < 1200; ++f) {
        Frame(controller, in);
        const auto pos = world.GetPlayer()->GetPosition();
        // The player's 24x24 box must never strictly overlap the victim's
        // 24x24 box on the shared row: its left edge stays >= vx (flush),
        // never crossing to the far side of the NPC.
        const bool sameRow = !(pos.y >= vy + 24.0f || pos.y + 24.0f <= vy);
        if (sameRow) CHECK(pos.x >= vx);               // no pass-through
    }
    const float endX = world.GetPlayer()->GetPosition().x;
    CHECK(endX >= vx);                                  // ended flush, not past

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I5 — a Vendor interaction must route to TryBuy via the choice UI. We
// drive the Ch4 集英樓 vendor (sells the ugly umbrella, setsFlag
// Flag_BoughtUglyUmbrella → Ending C). E to open the buy menu, E to
// confirm the only stock line; the purchase event fires, money is
// deducted, the inventory + the ending flag update — all inside
// Vendor::TryBuy, just as the pinned test_vendor contract requires.
TEST_CASE("I5: Vendor interaction routes to TryBuy (Ch4 ugly umbrella)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    int pickupHits = 0;
    std::string lastPickup;
    EventBus::Instance().Subscribe(EventType::PickupAcquired,
        [&](const Event& e) { ++pickupHits; lastPickup = e.text; });

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    // Drive the FSM to Ch4; GameController respawns the roster (which
    // includes the 集英樓 Vendor) on the next Update().
    world.Semester().Transition(SemesterState::Chapter4_Finals);
    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                             // roster -> Ch4

    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
    const float vx = vend->GetPosition().x;
    const float vy = vend->GetPosition().y;
    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    p->AddMoney(300);                                  // afford the 100 umbrella
    const int money0 = p->GetMoney();
    CHECK_FALSE(p->HasFlag("Flag_BoughtUglyUmbrella"));

    // Teleport adjacent (this test targets the BUY wiring, not pathing;
    // the I3 cases already prove walk-up+E reaches the dialog).
    p->SetPosition(nccu::gfx::Vec2{vx - 8.0f, vy});

    in.Tap(Key::E);                                    // open the buy menu
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());

    // Page through the greeting line(s) to the stock choice.
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());
    REQUIRE(world.Dialog().Choices().size() == 1);     // one stock line

    in.Tap(Key::E);                                    // confirm the buy
    Frame(controller, in);

    CHECK(p->GetMoney() == money0 - 100);              // soft-cap economy intact
    CHECK(p->HasFlag("Flag_BoughtUglyUmbrella"));      // Ending C key set
    CHECK(p->ConsumableCount("UglyUmbrella") == 1);    // inventory updated
    CHECK(pickupHits == 1);                            // EventBus purchase event
    CHECK(lastPickup == "UglyUmbrella");

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I5 — the Ch2 spine. The clear needs an inventory EnergyDrink to wake
// 學霸; only a Vendor buy can supply it. Buy at the Ch2 自動販賣機, then
// (with the 3 notes pre-set) talk to 學霸: TryRescueBookworm consumes the
// drink, sets Flag_BookwormRecovered, and LiftChapter2Clear sets
// Flag_Ch2Cleared once the thanks dialog closes. Before the I5 fix the
// EnergyDrink is unobtainable in-engine so Flag_Ch2Cleared is unreachable.
TEST_CASE("I5: Ch2 progression — buy EnergyDrink, wake 學霸, Flag_Ch2Cleared") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world};

    world.Semester().Transition(SemesterState::Chapter2_Midterms);
    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    Frame(controller, in);                             // roster -> Ch2

    Player* p = world.GetPlayer();
    REQUIRE(p != nullptr);
    // The 3 散落筆記 are an orthogonal pickup quest; pre-set them so this
    // test isolates the I5 EnergyDrink-supply defect (the rescue needs
    // notes-complete AND a drink — we exercise the drink path).
    p->SetFlag("Flag_FoundNote1");
    p->SetFlag("Flag_FoundNote2");
    p->SetFlag("Flag_FoundNote3");
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // none until bought

    // --- Buy the EnergyDrink at the Ch2 自動販賣機 vendor. ---
    const GameObject* vend = FindVendor(world);
    REQUIRE(vend != nullptr);
    p->SetPosition(nccu::gfx::Vec2{vend->GetPosition().x - 8.0f,
                                   vend->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    REQUIRE(world.Dialog().Active());
    for (int f = 0; f < 16 && !world.Dialog().AtChoice(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    REQUIRE(world.Dialog().AtChoice());
    in.Tap(Key::E);                                    // confirm buy
    Frame(controller, in);
    CHECK(p->ConsumableCount("EnergyDrink") == 1);     // <-- I5 supply fixed

    // --- Talk to 學霸: consumes the drink -> Flag_BookwormRecovered. ---
    const GameObject* bw = FindNpc(world, "bookworm");
    REQUIRE(bw != nullptr);
    p->SetPosition(nccu::gfx::Vec2{bw->GetPosition().x - 8.0f,
                                   bw->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag("Flag_BookwormRecovered"));
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // drink consumed

    // Close the 學霸 (d) thanks dialog; LiftChapter2Clear then sets
    // Flag_Ch2Cleared on the next non-dialog frame, the spine's Ch2 clear.
    for (int f = 0; f < 32 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }
    CHECK_FALSE(world.Dialog().Active());
    Frame(controller, in);                             // LiftChapter2Clear
    CHECK(p->HasFlag("Flag_Ch2Cleared"));              // <-- Ch2 spine clears

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}
