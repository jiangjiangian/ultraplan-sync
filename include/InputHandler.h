#ifndef INPUT_HANDLER_H_
#define INPUT_HANDLER_H_
#include "gfx/Input.h"
#include "gfx/Key.h"

namespace nccu {

// Cycle 10.P0a (awsome_cpp.md §6): the per-frame input layer extracted
// from GameController so the Controller stays the orchestrator and a
// focused class owns the input edge timing.
//
// Today only the dialog hold-E auto-advance (audit H2 / D5 / SC 2.2.2)
// carries cross-frame state — every other reader of gfx::Input goes
// through stateless edge predicates (IsPressed / IsDown / IsReleased)
// and is fine left at its call site. Folding the dialog auto-advance
// timer into a dedicated class:
//   * pins the input contract (edge-E or held-E ≥ 300 ms with a 4-frame
//     cooldown between auto-fires) in one place a regression test can
//     drive without spinning up the entire Controller / World stack.
//   * keeps the rest of the Update() loop free of mutable hold-state
//     scattered across the controller.
//   * lets future input refactors (rebinding, multi-source merging) live
//     here rather than continuing to bloat the Controller.
//
// Pure input timing: NO World mutation, no event publishing — the
// caller decides what to do with the boolean return. Stateless wrt the
// rest of the simulation, so a doctest can drive it with a synthetic
// InputSource (the harness's ScriptInput shape) and assert the exact
// edge / cooldown semantics.
class InputHandler {
public:
    InputHandler() = default;

    // Did the player issue a dialog-advance request THIS frame?
    //
    // True on either:
    //   * the raylib edge IsPressed(E) — i.e. the same one-frame tap that
    //     used to drive the dialog branch, semantically unchanged.
    //   * after holding E continuously for >= kHoldAdvanceMs and the
    //     internal cooldown has ticked back to zero. Each auto-fire
    //     re-arms the cooldown so the player can read each page rather
    //     than blinking through 60 pages/sec.
    //
    // Idempotent within a single frame: a second call in the same tick
    // returns the same boolean (no internal mutation outside of the
    // hold-timer increment, which is driven by `dt` exactly once per
    // call site — the controller calls this at most once per frame).
    //
    // dt: frame delta in seconds, sourced from gfx::Time::DeltaSeconds().
    bool TickDialogAdvance(float dt) noexcept;

    // The dialog has just closed (or never opened this frame). Drop the
    // hold timer so the next time a dialog opens, the player needs to
    // hold E afresh — a stale 300 ms read from before the dialog box
    // appeared must not auto-trigger on the very first frame of the new
    // conversation. Cheap; idempotent.
    void ResetDialogAdvance() noexcept {
        eHoldMs_ = 0.0f;
        eAutoAdvanceCooldown_ = 0;
    }

    // Test seam: how long (ms) the player must hold E continuously
    // before auto-advance arms. Pinned by the regression test.
    static constexpr float kHoldAdvanceMs = 300.0f;
    // Test seam: how many frames the auto-advance is silenced after
    // each fire, so the cadence stays readable.
    static constexpr int   kAutoCooldownFrames = 4;

    // Test-only inspection (no behavior dependency in production).
    [[nodiscard]] float HoldMs() const noexcept { return eHoldMs_; }
    [[nodiscard]] int   Cooldown() const noexcept {
        return eAutoAdvanceCooldown_;
    }

private:
    // ms E has been held; reset on release. Same shape as the previous
    // GameController::eHoldMs_ — moved here verbatim.
    float eHoldMs_ = 0.0f;
    // Frames to skip after each auto-fire. Same shape as the previous
    // GameController::eAutoAdvanceCooldown_.
    int   eAutoAdvanceCooldown_ = 0;
};

} // namespace nccu

#endif // INPUT_HANDLER_H_
