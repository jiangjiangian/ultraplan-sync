// Regression guard for BUGLEDGER I6 — the harness `interact <id>` plan
// verb could never press E, so every plan-driven playthrough soft-locked
// in Ch1 (no NPC dialog, no flags, no ending).
//
// Root cause (verified from source, not a guess): for a BlocksMovement()
// NPC the movement collider is a player-sized box AT the NPC origin and
// physics::ResolveMove (include/Physics.h:50-52, Rect::Intersects strict)
// halts the player EXACTLY flush against it — touching, never strictly
// overlapping. The pre-fix Verb::Interact aimed travel at npos.x-8 (the
// NPC's NEAR side, i.e. the far side of that wall) and only pressed E
// when AxisKeyToward returned -1 (arrived within epsilon). The player
// flush-stops ~24 px away from that target, so AxisKeyToward returns the
// approach key FOREVER, the press-E line is never reached, the watchdog
// abandons the step and the dialog never opens.
//
// The fix mirrors GameController's landed I3 condition: drive straight at
// the NPC ORIGIN and gate the E press on the SAME inflated-AABB overlap
// test (kInteractReach = 8) GameController.cpp:293-300 uses, so a
// wall-blocked walked-up player still talks — exactly the human-play
// geometry I3 relies on.
//
// Revert-verify (this test MUST FAIL without the production fix): restore
// Verb::Interact's travel target to `npos.x - 8` and gate the E press on
// `AxisKeyToward(...) < 0` (the pre-I6 logic) — "interact victim" then
// never opens the dialog (the player flush-stops ~24 px short of the
// near-side target, AxisKeyToward never returns -1, E is never pressed).
//
// Driven through the EXACT path the harness uses: ScriptInput is the
// gfx::Input source and the per-frame loop mirrors Harness
// (Advance + ResolvePlan against the previous frame's World snapshot,
// then controller.Update) so this exercises the production seam, not a
// unit shim. Headless, deterministic, no GL.

#include "doctest/doctest.h"
#include "engine/platform/ScriptInput.h"
#include "game/controller/GameController.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "engine/core/GameObject.h"
#include "engine/input/Input.h"
#include "engine/platform/Time.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::ScriptInput;
using nccu::World;

namespace {

const GameObject* FindNpc(const World& w, const char* id) {
    for (const auto& u : w.Objects())
        if (u && u->IsActive() && u->NpcId() == id) return u.get();
    return nullptr;
}

// One observable per frame: did the dialog open, and where did the player
// end up. Mirrors Harness::BeginFrame/EndFrame ordering exactly (the plan
// resolves against the World captured at the PREVIOUS frame's EndFrame).
struct Outcome {
    bool  dialogOpened = false;
    std::string npc;
    int   openedAtFrame = -1;
    float endX = 0.0f, endY = 0.0f;
    float startX = 0.0f;
};

// 善有善報 redesign: the Ch1 苦主 moved from the (380,1860) spawn row to
// 綜合院館 (1660,1010), north of the south-campus wall. The harness
// `interact <id>` verb is a pure X-then-Y axis driver (NOT a path-finder),
// so it can no longer drive straight up to the victim — it must first be
// routed through the single gap column (BUGLEDGER I7's routing model). This
// gap route (auto-derived + mask-verified via
// `.claude/tools/map_registry.py --route "500,1860 1660,1120"`) stages the
// player just south of the victim on the clear x=1660 column; the final
// `interact victim` then does the flush-approach + E that I6 guards.
const char* const kRouteToVictimStaging =
    "goto 1040 1712\n" "goto 1048 1704\n" "goto 1264 1632\n"
    "goto 1280 1624\n" "goto 1296 1616\n" "goto 1312 1608\n"
    "goto 1328 1600\n" "goto 1408 1512\n" "goto 1416 1504\n"
    "goto 1424 1496\n" "goto 1432 1488\n" "goto 1448 1480\n"
    "goto 1496 1456\n" "goto 1504 1448\n" "goto 1512 1440\n"
    "goto 1520 1432\n" "goto 1528 1424\n" "goto 1544 1328\n"
    "goto 1552 1320\n" "goto 1568 1312\n" "goto 1584 1304\n"
    "goto 1592 1296\n" "goto 1660 1120\n";

Outcome RunInteract(const char* npcId, int maxFrames,
                    const char* preRoute = kRouteToVictimStaging) {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    nccu::gfx::Time::SetFixedStep(1.0f / 60.0f);
    EventBus::Instance().Clear();

    World world("", /*loadSprites=*/false);
    nccu::GameController controller{world, EventBus::Instance()};

    std::string script = preRoute ? preRoute : "";
    script += "interact ";
    script += npcId;
    script += '\n';
    ScriptInput in;
    std::istringstream src(script);
    in.Load(src);
    nccu::gfx::Input::SetSource(&in);

    Outcome out;
    const Player* p0 = world.GetPlayer();
    out.startX = p0 ? p0->GetPosition().x : 0.0f;

    const World* snapshot = nullptr;     // null on frame 0, like the harness
    for (int f = 0; f < maxFrames; ++f) {
        in.Advance();
        in.ResolvePlan(snapshot);
        controller.Update();
        snapshot = &world;

        const auto& d = world.Dialog();
        if (d.Active() && !out.dialogOpened) {
            out.dialogOpened = true;
            out.npc = std::string(d.NpcId());
            out.openedAtFrame = f;
        }
        if (in.WantsQuit() || (in.HasPlan() && f >= 1 && in.PlanDone()))
            break;
    }
    const Player* p = world.GetPlayer();
    out.endX = p ? p->GetPosition().x : 0.0f;
    out.endY = p ? p->GetPosition().y : 0.0f;

    nccu::gfx::Input::SetSource(nullptr);
    nccu::gfx::Time::SetFixedStep(0.0f);
    EventBus::Instance().Clear();
    return out;
}

}  // namespace

// The headline I6 lock. The Ch1 苦主 is a BlocksMovement() NPC; `interact
// victim` walks the player into it, the movement collider flush-stops the
// player TOUCHING the NPC (never overlapping), and the verb MUST still
// press E so the conversation opens. Pre-fix this soft-locks: the press-E
// line is unreachable and the dialog never opens (the whole A/B/C spine
// dies here in Ch1). 善有善報: the victim is now at 綜合院館 (1660,1010);
// RunInteract first routes through the gap to a staging point just SOUTH
// of him on the clear x=1660 column, so the final flush-approach is along
// the Y axis (northward), aligned on X.
TEST_CASE("I6: harness `interact victim` opens the NPC dialog (flush-blocked)") {
    World probe("", /*loadSprites=*/false);
    const GameObject* v = FindNpc(probe, "victim");
    REQUIRE(v != nullptr);
    const float vx = v->GetPosition().x;             // {1660,1010}
    const float vy = v->GetPosition().y;

    const Outcome o = RunInteract("victim", 3000);

    // The verb actually pressed E and the game opened the 苦主 dialog —
    // the exact thing the pre-fix code could never do.
    CHECK(o.dialogOpened);                            // <-- the I6 lock
    CHECK(o.npc == "victim");
    CHECK(o.openedAtFrame >= 0);

    // It reached the NPC the way a human walks up: flush against (never
    // through) the 24x24 movement collider on the approach axis (Y here),
    // aligned on the other (X). Proves the press fired from the I3-mirrored
    // reach geometry, not from teleporting onto the NPC. The staging point
    // is south of the victim, so the player ends BELOW him (endY >= vy).
    CHECK(o.endY >= vy);                              // never walked through
    CHECK(std::fabs(o.endY - vy) <= 24.0f + 8.0f);    // within reach margin
    // Aligned on X to within the goto arrive-epsilon (the staging hop lands
    // within ~2 px of the x=1660 column, then the origin-seeking drive holds
    // it there); pre-redesign this was 0 only because the victim sat exactly
    // on the spawn row (no staging hop).
    CHECK(std::fabs(o.endX - vx) <= 2.0f);            // Y-axis approach column
}

// Companion: the routed approach + origin-seeking drive is a pure function
// of (plan step, World snapshot), so two runs of the SAME gap route +
// `interact victim` produce a byte-identical trace (open frame, end pos).
TEST_CASE("I6: `interact` opens dialog deterministically (two runs identical)") {
    const Outcome a = RunInteract("victim", 3000);
    const Outcome b = RunInteract("victim", 3000);

    CHECK(a.dialogOpened);
    CHECK(b.dialogOpened);
    // Pure function of (plan step, World snapshot): byte-identical trace.
    CHECK(a.dialogOpened   == b.dialogOpened);
    CHECK(a.npc            == b.npc);
    CHECK(a.openedAtFrame  == b.openedAtFrame);
    CHECK(a.endX           == b.endX);
    CHECK(a.endY           == b.endY);
}
