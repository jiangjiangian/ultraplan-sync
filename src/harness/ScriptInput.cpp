#include "harness/ScriptInput.h"

#include "world/World.h"
#include "entities/Player.h"
#include "entities/GameObject.h"
#include "dialog/DialogState.h"
#include "engine/input/Key.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "raylib.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace nccu {
namespace {

int KeyCode(std::string_view tok) {
    using nccu::gfx::Key;
    if (tok.size() == 1 && tok[0] >= 'A' && tok[0] <= 'Z')
        return KEY_A + (tok[0] - 'A');
    if (tok == "Enter")     return ToRaylibKey(Key::Enter);
    if (tok == "Escape")    return ToRaylibKey(Key::Escape);
    if (tok == "Tab")       return ToRaylibKey(Key::Tab);
    if (tok == "Space")     return ToRaylibKey(Key::Space);
    if (tok == "Backspace") return ToRaylibKey(Key::Backspace);
    if (tok == "Up")        return ToRaylibKey(Key::Up);
    if (tok == "Down")      return ToRaylibKey(Key::Down);
    if (tok == "Left")      return ToRaylibKey(Key::Left);
    if (tok == "Right")     return ToRaylibKey(Key::Right);
    return -1;
}

// A line is a high-level verb iff its first non-space char is not a digit
// and not '-' (no negative frame numbers exist) — keeps the classic
// "<frame> ..." grammar working untouched (additive, never ambiguous).
bool LooksLikeVerb(const std::string& line) {
    for (char c : line) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        if (c == '#') return false;                 // comment
        return !(std::isdigit(static_cast<unsigned char>(c)) || c == '-');
    }
    return false;                                   // blank
}

// Movement constants — derived from gameplay, asserted-not-guessed:
// Player speed 180 px/s at the harness's pinned 1/60 s fixed step =>
// exactly 3.0 px/frame axis-aligned (see Player ctor / Character::Move).
constexpr float kPxPerFrame = 3.0f;
// Arrive when both axes are within this band. < one frame's travel so the
// player never oscillates around the target (it stops, doesn't overshoot
// past the epsilon then come back). Deterministic float compare.
constexpr float kArriveEps  = 2.0f;
// Bounded-progress guards (deterministic frame budgets, never wall-clock):
// generous vs. the longest campus traversal yet finite so a mis-aimed
// verb fails the run instead of hanging it.
constexpr int kGotoBudget     = 4000;   // ~66 s of pure travel
constexpr int kInteractBudget = 4200;   // travel + a few E taps
constexpr int kChooseBudget   = 64;     // cursor steps + confirm
constexpr int kAdvanceBudget  = 8;      // E taps to turn one page/line

} // namespace

void ScriptInput::Load(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (LooksLikeVerb(line)) {
            std::istringstream ls(line);
            std::string verb;
            ls >> verb;
            if (verb == "goto") {
                Step s; s.verb = Verb::Goto;
                if (ls >> s.x >> s.y) plan_.push_back(std::move(s));
            } else if (verb == "interact") {
                // `interact <npcId>`                — talk to / pick up
                //                                     the object with
                //                                     that NpcId.
                // `interact <label> <x> <y>`        — drive to world
                //                                     (x,y) and tap E
                //                                     there. The harness
                //                                     plan has no other
                //                                     way to E-actuate a
                //                                     non-NPC world
                //                                     object (umbrellas /
                //                                     QuestFlagPickups
                //                                     have an empty
                //                                     NpcId(), so they
                //                                     are unreachable by
                //                                     the NPC form — the
                //                                     whole A/B/C spine
                //                                     needs this to claim
                //                                     the TrueUmbrella /
                //                                     the 申請書). <label>
                //                                     is a human-readable
                //                                     comment token only;
                //                                     the coords are the
                //                                     actual target. Edge
                //                                     case: an NpcId that
                //                                     also has coords
                //                                     still resolves as
                //                                     the NPC (coords
                //                                     ignored) so the
                //                                     existing form is
                //                                     byte-unchanged.
                Step s; s.verb = Verb::Interact;
                if (ls >> s.arg) {
                    float cx, cy;
                    if (ls >> cx >> cy) { s.x = cx; s.y = cy; s.n = 1; }
                    plan_.push_back(std::move(s));
                }
            } else if (verb == "choose") {
                Step s; s.verb = Verb::Choose;
                if (ls >> s.n && s.n >= 0) plan_.push_back(std::move(s));
            } else if (verb == "advance") {
                Step s; s.verb = Verb::Advance;
                plan_.push_back(std::move(s));
            } else if (verb == "wait") {
                Step s; s.verb = Verb::Wait;
                if (ls >> s.n && s.n > 0) plan_.push_back(std::move(s));
            } else if (verb == "quit") {
                Step s; s.verb = Verb::Quit;
                plan_.push_back(std::move(s));
            }
            continue;                               // not a timed directive
        }
        std::istringstream ls(line);
        int frame;
        std::string verb, key;
        if (!(ls >> frame)) continue;               // blank / comment / '#'
        if (!(ls >> verb)) continue;
        if (verb == "quit") {
            byFrame_[frame].push_back({Directive::Quit, -1});
            continue;
        }
        if (!(ls >> key)) continue;
        const int kc = KeyCode(key);
        if (kc < 0) continue;
        if (verb == "down")       byFrame_[frame].push_back({Directive::Down, kc});
        else if (verb == "up")    byFrame_[frame].push_back({Directive::Up, kc});
        else if (verb == "press") byFrame_[frame].push_back({Directive::Press, kc});
    }
}

void ScriptInput::LoadFile(const std::string& path) {
    std::ifstream in(path);
    Load(in);
}

void ScriptInput::Advance() {
    ++frame_;
    pressed_.clear();
    released_.clear();
    for (int k : autoUp_)
        if (down_.erase(k)) released_.insert(k);
    autoUp_.clear();

    auto it = byFrame_.find(frame_);
    if (it == byFrame_.end()) return;
    for (const Directive& d : it->second) {
        switch (d.kind) {
            case Directive::Down:  SynthDown(d.key);  break;
            case Directive::Up:    SynthUp(d.key);    break;
            case Directive::Press: SynthPress(d.key); break;
            case Directive::Quit:  quit_ = true;      break;
        }
    }
}

// --- synthetic edge helpers (shared by classic + plan) ------------------
void ScriptInput::SynthDown(int key) {
    if (down_.insert(key).second) pressed_.insert(key);
}
void ScriptInput::SynthUp(int key) {
    if (down_.erase(key)) released_.insert(key);
}
void ScriptInput::SynthPress(int key) {
    down_.insert(key);
    pressed_.insert(key);
    autoUp_.push_back(key);
}

namespace {

// Hold exactly one axis key toward (tx,ty) from (px,py): X first, then Y
// (axis-aligned, mirrors hand scripts). Returns the raylib keycode to
// hold this frame, or -1 when already within epsilon on both axes.
int AxisKeyToward(float px, float py, float tx, float ty) {
    using nccu::gfx::Key;
    const float dx = tx - px;
    const float dy = ty - py;
    if (std::fabs(dx) >= kArriveEps)
        return dx > 0.0f ? ToRaylibKey(Key::D) : ToRaylibKey(Key::A);
    if (std::fabs(dy) >= kArriveEps)
        return dy > 0.0f ? ToRaylibKey(Key::S) : ToRaylibKey(Key::W);
    return -1;
}

const GameObject* FindNpc(const World& w, std::string_view id) {
    for (const auto& up : w.Objects()) {
        if (!up || !up->IsActive()) continue;
        if (up->NpcId() == id) return up.get();
    }
    return nullptr;
}

} // namespace

void ScriptInput::ResolvePlan(const World* world) {
    using nccu::gfx::Key;

    // A classic-only script has NO plan verbs (plan_ is empty). It governs
    // WASD purely via timed `down`/`up`/`press` directives that Advance()
    // applied this same frame (Advance runs before ResolvePlan). The plan
    // resolver owns no keys here, so it must not touch key state at all —
    // returning before releaseMoveKeys() leaves the classic directives'
    // held/pressed edges intact (BUGLEDGER I4: an unconditional
    // releaseMoveKeys() here SynthUp'd W/A/S/D every frame and silently
    // killed all classic harness movement).
    if (plan_.empty()) return;

    static const int kMoveKeys[4] = {
        ToRaylibKey(Key::W), ToRaylibKey(Key::A),
        ToRaylibKey(Key::S), ToRaylibKey(Key::D)};

    // Release any movement key the plan was holding; each verb re-asserts
    // the one it wants this frame. (Only reached for plan-bearing scripts;
    // a classic-only script returned above untouched. A script that mixes
    // a plan verb and a classic directive for the same WASD key is its own
    // concern — the plan owns WASD while a verb is active.)
    auto releaseMoveKeys = [&] {
        for (int k : kMoveKeys) SynthUp(k);
    };

    if (planPc_ >= plan_.size()) { releaseMoveKeys(); return; }
    if (!world) { releaseMoveKeys(); return; }      // no snapshot yet: idle

    const Step& step = plan_[planPc_];

    auto finishStep = [&] {
        releaseMoveKeys();
        ++planPc_;
        planSub_      = 0;
        planWatchdog_ = 0;
    };

    switch (step.verb) {
        case Verb::Quit:
            quit_ = true;
            finishStep();
            return;

        case Verb::Wait:
            releaseMoveKeys();
            if (++planSub_ >= step.n) finishStep();
            return;

        case Verb::Goto: {
            const Player* p = world->GetPlayer();
            if (!p) { releaseMoveKeys(); return; }
            const auto pos = p->GetPosition();
            const int k = AxisKeyToward(pos.x, pos.y, step.x, step.y);
            releaseMoveKeys();
            if (k < 0) { finishStep(); return; }    // arrived
            SynthDown(k);
            if (++planWatchdog_ >= kGotoBudget) finishStep();  // bounded
            return;
        }

        case Verb::Interact: {
            const Player* p = world->GetPlayer();
            const GameObject* npc = FindNpc(*world, step.arg);
            if (!p) { releaseMoveKeys(); finishStep(); return; }
            // No NPC by that id. If the step carries a world coordinate
            // (the `interact <label> <x> <y>` form), this is a non-NPC
            // E-actuation (claim the TrueUmbrella / pick up the 申請書 /
            // open a Vendor menu): drive to (x,y) and tap E once the
            // I3-mirrored reach probe overlaps a STANDARD 24x24 object
            // footprint at the target. The 24x24 box (not a 1x1 point)
            // is essential for a movement-BLOCKING target (a Vendor):
            // physics flush-stops the player ~24 px from the object
            // ORIGIN, so a point gate would never fire (the I6 class of
            // bug). A 24x24-at-origin box + the 8 px reach margin is the
            // exact geometry GameController's real E-probe vs. the real
            // hitbox uses, and it is conservative for a non-blocking item
            // (the player walks ONTO it, well inside this box). No coords
            // + no NPC ⇒ unknown id, skip the step (unchanged).
            if (!npc) {
                if (step.n != 1) { releaseMoveKeys(); finishStep(); return; }
                // A Vendor menu (or any dialog) is now open ⇒ the
                // E-actuation landed; hand straight off (like the NPC
                // arm's dialog-active early-out) so a following `choose`/
                // `advance` drives the menu. WITHOUT this, the E-tap loop
                // below would keep pressing E into the open Vendor menu
                // and page/confirm it (double-buy observed). A silent
                // pickup (umbrella / coin / note) opens no dialog, so it
                // still falls through to the tap path.
                if (world->Dialog().Active()) { finishStep(); return; }
                const auto pp = p->GetPosition();
                // Cycle 9.E (audit M2 / D7 / SC 2.5.8): mirror the
                // GameController E-probe reach so the harness's "I'm
                // close enough to press E now" gate stays byte-identical
                // to the engine's "the press would land" gate. Reading
                // World::LargeTargets() (set by UMBRELLA_LARGE_TARGETS=1)
                // keeps the script's E-press timing correct under the
                // accessibility profile too. Default off ⇒ 8.0f, matching
                // every harness regression pinned before this profile.
                const float kInteractReach =
                    world->LargeTargets() ? 16.0f : 8.0f;
                const gfx::Rect pHit{pp.x - kInteractReach,
                                     pp.y - kInteractReach,
                                     24.0f + 2.0f * kInteractReach,
                                     24.0f + 2.0f * kInteractReach};
                // Generous 24x24-at-origin box just decides WHEN to start
                // tapping E (it must, for a movement-blocking Vendor that
                // flush-stops the player ~24 px from the origin — the I6
                // class). It is intentionally NOT the completion test:
                // the real GameController E-probe is the player box vs the
                // object's ACTUAL hitbox (16x16 QuestFlagPickup, 20x20
                // umbrella, 24x24 Vendor), so a 24x24 script box can be
                // in "reach" while the real test is still false. So we
                // keep DRIVING at the origin AND tapping E every frame,
                // and only hand off once the player has actually ARRIVED
                // at the origin (k < 0 ⇒ on a non-blocking item, every
                // real hitbox now overlaps and it was claimed mid-walk)
                // — or, for a Vendor, the moment its menu opens (the
                // dialog-active guard above, next frame). The travel
                // watchdog bounds an unreachable target.
                const gfx::Rect tgt{step.x, step.y, 24.0f, 24.0f};
                const int k = AxisKeyToward(pp.x, pp.y, step.x, step.y);
                releaseMoveKeys();
                if (k >= 0) SynthDown(k);             // drive / hold flush
                if (pHit.Intersects(tgt))             // within E reach
                    SynthPress(ToRaylibKey(Key::E));
                if (k < 0) { finishStep(); return; }  // arrived ⇒ claimed
                if (++planWatchdog_ >= kInteractBudget) finishStep();
                return;
            }
            // Already in dialog with content open? interact is satisfied.
            if (world->Dialog().Active()) { finishStep(); return; }

            const auto pos  = p->GetPosition();
            const auto npos = npc->GetPosition();
            // I6 fix: the press-E gate must be the SAME inflated-AABB
            // overlap test GameController's I3 fix uses, NOT "AxisKeyToward
            // returned -1". For a BlocksMovement() NPC the movement
            // collider is a player-sized box at the NPC origin and
            // physics::ResolveMove halts the player EXACTLY flush against
            // it (touching, never overlapping). So aiming travel at any
            // point on the NPC's near side (the old npos.x-8 target) lands
            // on the far side of that wall — unreachable — and an arrival
            // test would never fire (the I6 soft-lock). Instead: walk
            // straight at the NPC ORIGIN; the collider flush-stops the
            // player touching the NPC, and an interaction-reach-inflated
            // probe (mirroring GameController.cpp:293-297) then overlaps
            // the NPC hitbox so we press E — exactly the human-play
            // geometry the landed I3 fix relies on. Pure function of the
            // two live positions => deterministic.
            // Cycle 9.E (audit M2 / D7 / SC 2.5.8): like the non-NPC arm
            // above, mirror the engine's larger-targets reach so the
            // harness's "press E now" gate tracks GameController's "the
            // press would land" gate under the accessibility profile too.
            const float kInteractReach =
                world->LargeTargets() ? 16.0f : 8.0f;
            const gfx::Rect pHit{pos.x - kInteractReach,
                                 pos.y - kInteractReach,
                                 24.0f + 2.0f * kInteractReach,
                                 24.0f + 2.0f * kInteractReach};
            // Drive straight at the NPC ORIGIN every frame until dialog
            // opens. AxisKeyToward closes X then Y, so a row-misaligned
            // start still converges onto the NPC's row before pinning on
            // X. The movement collider (a player-sized box at the NPC
            // origin) flush-stops the player TOUCHING the NPC — it can
            // never walk through (the I3 invariant) — so keeping the key
            // held while in reach just pins the player flush, it does not
            // overshoot. This mirrors how a human walks up: keep pushing
            // toward the NPC while mashing E.
            const int k = AxisKeyToward(pos.x, pos.y, npos.x, npos.y);
            releaseMoveKeys();
            if (k >= 0) SynthDown(k);                 // travel / hold flush
            // Press-E gate = the SAME inflated-AABB overlap test
            // GameController's I3 fix uses (NOT "AxisKeyToward returned
            // -1": targeting the origin flush-stops the player ~24 px
            // away, so an arrival test would reproduce the I6 soft-lock).
            // The 8 px reach margin covers the flush-touching gap so this
            // fires for a wall-blocked player — exactly the human-play
            // geometry GameController.cpp:293-300 relies on. While we are
            // still in reach the held key keeps walking the player into
            // the flush stop (it cannot pass), so it ends adjacent.
            if (npc->CheckCollision(pHit))           // within talk reach
                SynthPress(ToRaylibKey(Key::E));
            if (++planWatchdog_ >= kInteractBudget) finishStep();
            return;
        }

        case Verb::Choose: {
            releaseMoveKeys();
            const DialogState& d = world->Dialog();
            if (!d.Active()) { finishStep(); return; } // nothing to choose
            if (!d.AtChoice()) {                       // page through lines
                SynthPress(ToRaylibKey(Key::E));
                if (++planWatchdog_ >= kChooseBudget) finishStep();
                return;
            }
            const int cur = d.ChoiceCursor();
            if (cur != step.n) {                        // step the cursor
                SynthPress(cur < step.n ? ToRaylibKey(Key::Down)
                                        : ToRaylibKey(Key::Up));
                if (++planWatchdog_ >= kChooseBudget) finishStep();
                return;
            }
            SynthPress(ToRaylibKey(Key::E));            // confirm
            finishStep();
            return;
        }

        case Verb::Advance: {
            releaseMoveKeys();
            const DialogState& d = world->Dialog();
            if (!d.Active()) { finishStep(); return; } // nothing to advance
            // One E tap moves DialogState by exactly one step — turn a
            // page, step a line, or close the box (verified in
            // DialogState::Advance). So a single deterministic tap IS the
            // unit of "advance the line/page progresses or closes". The
            // controller consumes the edge next frame; we hand off then.
            // (void) the budget constant to keep the bounded-by-design
            // contract explicit without an unused-symbol warning.
            static_assert(kAdvanceBudget > 0, "advance is bounded by design");
            SynthPress(ToRaylibKey(Key::E));
            finishStep();
            return;
        }
    }
}

bool ScriptInput::IsDown(gfx::Key k) const noexcept {
    return down_.find(ToRaylibKey(k)) != down_.end();
}
bool ScriptInput::IsPressed(gfx::Key k) const noexcept {
    return pressed_.find(ToRaylibKey(k)) != pressed_.end();
}
bool ScriptInput::IsReleased(gfx::Key k) const noexcept {
    return released_.find(ToRaylibKey(k)) != released_.end();
}

} // namespace nccu
