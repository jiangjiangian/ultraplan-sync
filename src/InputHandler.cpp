#include "InputHandler.h"

namespace nccu {

bool InputHandler::TickDialogAdvance(float dt) noexcept {
    using nccu::gfx::Input;
    using nccu::gfx::Key;

    // Hold timer: ticked while E is held, snapped to 0 on release. The
    // cooldown counter also resets on release so a future hold starts
    // fresh — moved verbatim from the previous GameController body.
    if (Input::IsDown(Key::E)) {
        eHoldMs_ += dt * 1000.0f;
    } else {
        eHoldMs_ = 0.0f;
        eAutoAdvanceCooldown_ = 0;
    }

    const bool edgeE = Input::IsPressed(Key::E);

    // Hold-fire branch ONLY when E is held but NOT freshly pressed. The
    // edge-press path below covers the tap-once-per-press semantics so
    // a single press never both edge-fires AND auto-fires on the same
    // frame (which would double-advance the dialog box).
    bool autoE = false;
    if (!edgeE && Input::IsDown(Key::E) && eHoldMs_ >= kHoldAdvanceMs) {
        if (eAutoAdvanceCooldown_ > 0) {
            --eAutoAdvanceCooldown_;
        } else {
            autoE = true;
            eAutoAdvanceCooldown_ = kAutoCooldownFrames;
        }
    }

    return edgeE || autoE;
}

} // namespace nccu
