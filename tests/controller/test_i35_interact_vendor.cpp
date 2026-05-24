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
#include "controller/GameController.h"
#include "world/World.h"
#include "entities/Player.h"
#include "dialog/DialogState.h"
#include "dialog/DialogSource.h"
#include "quest/ChapterVendors.h"
#include "quest/ItemCatalog.h"
#include "quest/Chapter2Quest.h"
#include "controller/EventBus.h"
#include "entities/GameObject.h"
#include "state/SemesterState.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"

#include <cmath>
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
    const float vx = v->GetPosition().x;             // {1660,1010}
    const float vy = v->GetPosition().y;

    // 善有善報: the victim moved to 綜合院館 (1660,1010), off the spawn row
    // and behind the south-campus wall. The I3 GUARANTEE (a flush-blocked
    // walked-up player can still talk via GameController's inflated E-probe)
    // is position-independent, so place the player just SOUTH of the victim
    // on the clear x=1660 column and walk UP into him — the same flush-block
    // + E-probe geometry, exercised on the Y axis instead of the X axis.
    world.GetPlayer()->SetPosition(nccu::gfx::Vec2{vx, vy + 90.0f});

    TestInput in;
    nccu::gfx::Input::SetSource(&in);

    const bool opened = WalkUpAndTalk(controller, in, world, Key::W, vx);

    // Flush against the victim: pre-fix this is where the spine dies. The
    // player can be touched-but-not-overlapped, so a 24x24 E-probe at the
    // player origin never collides. The interaction reach fix makes the
    // inflated probe overlap the NPC hitbox. Approaching from the south, the
    // player ends BELOW the victim (py >= vy), aligned on X.
    const float px = world.GetPlayer()->GetPosition().x;
    const float py = world.GetPlayer()->GetPosition().y;
    CHECK(py >= vy);                                   // never walked through
    CHECK(py <= vy + 40.0f);                            // truly adjacent
    CHECK(std::fabs(px - vx) < 1.0f);                   // Y-axis approach column
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

    // 善有善報: place the player just SOUTH of the victim (1660,1010) on the
    // clear column and shove UP into him — same no-pass-through guarantee on
    // the Y axis (the victim is no longer on the spawn row).
    world.GetPlayer()->SetPosition(nccu::gfx::Vec2{vx, vy + 90.0f});

    TestInput in;
    nccu::gfx::Input::SetSource(&in);
    in.Hold(Key::W);                                   // shove up, hard
    for (int f = 0; f < 1200; ++f) {
        Frame(controller, in);
        const auto pos = world.GetPlayer()->GetPosition();
        // The player's 24x24 box must never strictly overlap the victim's
        // 24x24 box on the shared column: its top edge stays >= vy (flush),
        // never crossing to the far side of the NPC.
        const bool sameCol = !(pos.x >= vx + 24.0f || pos.x + 24.0f <= vx);
        if (sameCol) CHECK(pos.y >= vy);               // no pass-through
    }
    const float endY = world.GetPlayer()->GetPosition().y;
    CHECK(endY >= vy);                                  // ended flush, not past

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
    // REQUIREMENT #4 reconciliation: the menu is now one stock line PLUS
    // a trailing "不買" decline choice (a forced purchase is a defect).
    // choiceCursor_ defaults to 0 (the stock line), so a single E still
    // confirms the BUY exactly as before — the I5 wiring is unchanged;
    // only the menu size grew by the decline entry. test_vendor_decline
    // .cpp owns the proof that picking the decline mutates nothing.
    REQUIRE(world.Dialog().Choices().size() == 2);     // stock + 不買
    CHECK(world.Dialog().Choices().back().label == "先不買，謝謝");
    CHECK(world.Dialog().ChoiceCursor() == 0);         // defaults to buy

    in.Tap(Key::E);                                    // confirm the buy
    Frame(controller, in);

    CHECK(p->GetMoney() == money0 - 100);              // soft-cap economy intact
    CHECK(p->HasFlag("Flag_BoughtUglyUmbrella"));      // Ending C key set
    // B2.1: the 醜傘 buy is now a HELD umbrella, not a count-consumable —
    // the player holds it (auto-shelter) and the bag shows a single ugly
    // umbrella row, with NO phantom "UglyUmbrella" consumable entry.
    CHECK(p->ConsumableCount("UglyUmbrella") == 0);    // not a consumable
    CHECK(p->HeldUmbrellaKind() == HeldUmbrella::Ugly);
    CHECK(p->HasUmbrella());                           // now sheltered
    CHECK_FALSE(p->HasFlag("Flag_HasTrueUmbrella"));   // NOT the true umbrella
    {
        const auto rows = nccu::BuildInventoryRows(*p);
        int umbRows = 0;
        for (const auto& r : rows)
            if (r.itemId == nccu::kItemUglyUmbrella) ++umbRows;
        CHECK(umbRows == 1);                           // exactly one, no double row
    }
    CHECK(pickupHits == 1);                            // EventBus purchase event
    CHECK(lastPickup == "UglyUmbrella");

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
}

// I5 — the Ch2 spine, two-phase flow. The clear needs an inventory
// EnergyDrink to WAKE 學霸 (only a Vendor buy can supply it). Buy at the
// Ch2 自動販賣機; talk to 學霸 once -> phase 1 consumes the drink and wakes
// him (Flag_Bookworm, which starts the note quest); with the 3 notes in,
// a second talk -> phase 2 exchanges (Flag_BookwormRecovered); closing the
// (d) thanks dialog lets LiftChapter2Clear set Flag_Ch2Cleared. Before the
// I5 fix the EnergyDrink was unobtainable in-engine so the spine stalled.
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

    // --- A2 (hard-gate): meet the 圖書館管理員 FIRST. She is the Ch2 chain
    // head — until Flag_MetLibrarian is set, the 學霸 cannot be woken (the
    // wake step refuses and the opener redirects to the 櫃台). Walk to her and
    // tap E to fire the real TryMeetLibrarian hook. ---
    const GameObject* lib = FindNpc(world, "librarian");
    REQUIRE(lib != nullptr);
    p->SetPosition(nccu::gfx::Vec2{lib->GetPosition().x - 8.0f,
                                   lib->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag(nccu::kFlagMetLibrarian));         // chain head met
    for (int f = 0; f < 16 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }

    // --- Talk to 學霸, PHASE 1: consumes the drink -> wakes him
    // (Flag_Bookworm). The new two-phase flow no longer recovers in one
    // step: waking is what starts the note quest. ---
    const GameObject* bw = FindNpc(world, "bookworm");
    REQUIRE(bw != nullptr);
    p->SetPosition(nccu::gfx::Vec2{bw->GetPosition().x - 8.0f,
                                   bw->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag("Flag_Bookworm"));                // woken
    CHECK(p->ConsumableCount("EnergyDrink") == 0);     // drink spent at wake
    CHECK_FALSE(p->HasFlag("Flag_BookwormRecovered")); // not yet — needs notes

    // Close the wake (c) dialog so the next E is a fresh interaction.
    for (int f = 0; f < 16 && world.Dialog().Active(); ++f) {
        in.Tap(Key::E);
        Frame(controller, in);
    }

    // --- Talk to 學霸, PHASE 2: notes are in (pre-set above) -> exchange:
    // Flag_BookwormRecovered. No further drink consumed. ---
    p->SetPosition(nccu::gfx::Vec2{bw->GetPosition().x - 8.0f,
                                   bw->GetPosition().y});
    in.Tap(Key::E);
    Frame(controller, in);
    CHECK(p->HasFlag("Flag_BookwormRecovered"));

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
