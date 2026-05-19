#ifndef SCRIPT_INPUT_H_
#define SCRIPT_INPUT_H_
#include "gfx/Input.h"

#include <istream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nccu {

class World;  // forward decl: the high-level plan reads World read-only

// Deterministic InputSource: replays a scripted key timeline so the game
// can be driven headless and reproducibly.
//
// TWO grammars, both in the same file, freely interleaved (a line is a
// high-level verb iff its first token is NON-numeric; otherwise it is a
// classic timed directive — fully backward compatible):
//
// A) Classic timed directives (one per line; '#'/blank ignored; 60 fps):
//      <frame> down  <KEY>     held from this frame on
//      <frame> up    <KEY>     released from this frame on
//      <frame> press <KEY>     one-frame tap (auto-released next frame)
//      <frame> quit            request a clean end at this frame
//    KEY: A..Z, Enter, Escape, Tab, Space, Up, Down, Left, Right.
//    Edge semantics mirror raylib: IsPressed only on the down frame,
//    IsReleased only on the up frame, IsDown for every frame between.
//
// B) High-level plan verbs (no leading frame; executed in file order,
//    one active at a time, each COMPILED into synthetic key edges per
//    frame so timelines are robust without frame-exact hand-counting):
//      goto <X> <Y>        drive the player toward world (X,Y) using the
//                          existing 3 px/frame axis movement; done within
//                          a small epsilon. Pure fn of player position.
//      interact <npcId>    look up that NPC's live World position, goto
//                          adjacent so the 24x24 hitboxes overlap, then
//                          hold E until World's dialog.active is true.
//      choose <index>      with a choice menu active, step the cursor to
//                          <index> (Up/Down) then confirm with E.
//      advance             tap the dialog-advance key (E) until the line/
//                          page progresses or the box closes (bounded).
//      wait <frames>       deterministic spacer: idle for <frames> frames.
//      quit                end the run cleanly once reached.
//
// Determinism: the plan is resolved every frame by ResolvePlan(World) as
// a PURE function of the current plan step and the World snapshot the
// harness captured at the previous EndFrame. No wall-clock, no RNG. Same
// script + same simulation => identical synthetic edges => byte-identical
// state.jsonl across runs (verified). Advance() steps exactly one frame
// and must be called once per game frame before input is read; the
// harness calls ResolvePlan() right after, before GameController reads
// input.
class ScriptInput final : public gfx::InputSource {
public:
    void Load(std::istream& in);
    void LoadFile(const std::string& path);

    // Step the classic timeline one frame. Call once per game frame.
    void Advance();
    // Resolve the active high-level plan step against a World snapshot,
    // injecting the synthetic key edges for THIS frame. world may be null
    // (e.g. before the first World snapshot): then the plan idles. Call
    // once per frame, AFTER Advance(), BEFORE the controller reads input.
    void ResolvePlan(const World* world);

    [[nodiscard]] bool WantsQuit() const noexcept { return quit_; }
    // Did the script declare any high-level plan verbs? Classic-only
    // scripts have none — the harness must keep governing those purely by
    // `quit`/maxframes (never auto-quit them).
    [[nodiscard]] bool HasPlan() const noexcept { return !plan_.empty(); }
    // True once every high-level plan verb has completed. Only meaningful
    // alongside HasPlan(): lets a plan-driven run end without a hand-
    // placed `quit`/maxframes once the last verb finishes.
    [[nodiscard]] bool PlanDone() const noexcept {
        return planPc_ >= plan_.size();
    }

    bool IsDown(gfx::Key k)     const noexcept override;
    bool IsPressed(gfx::Key k)  const noexcept override;
    bool IsReleased(gfx::Key k) const noexcept override;

private:
    struct Directive { enum Kind { Down, Up, Press, Quit } kind; int key; };

    enum class Verb { Goto, Interact, Choose, Advance, Wait, Quit };
    struct Step {
        Verb        verb;
        float       x = 0.0f, y = 0.0f;   // Goto target
        std::string arg;                  // Interact npcId
        int         n = 0;                // Choose index / Wait frames
    };

    // Apply one synthetic key edge into the live key sets (mirrors the
    // raylib edge rules the classic path uses), so a verb's keypress is
    // indistinguishable from a scripted one to GameController/Player.
    void SynthDown(int key);
    void SynthUp(int key);
    void SynthPress(int key);

    // --- classic timeline -------------------------------------------------
    std::unordered_map<int, std::vector<Directive>> byFrame_;
    std::vector<int> autoUp_;
    int  frame_ = -1;

    // --- shared key state (classic + plan both write here) ---------------
    std::unordered_set<int> down_, pressed_, released_;
    bool quit_ = false;

    // --- high-level plan --------------------------------------------------
    std::vector<Step> plan_;
    std::size_t       planPc_       = 0;     // program counter into plan_
    int               planSub_      = 0;     // per-verb sub-phase
    int               planWatchdog_ = 0;     // per-verb bounded-progress guard
};

} // namespace nccu

#endif // SCRIPT_INPUT_H_
