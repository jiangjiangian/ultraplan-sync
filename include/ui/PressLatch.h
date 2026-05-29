#ifndef UI_PRESS_LATCH_H_
#define UI_PRESS_LATCH_H_

namespace nccu {

// Edge-debounce for confirm / dismiss keys across blocking screens.
//
// The interactive title, character-select and help screens each run their
// own blocking draw-loop and read the SAME global key state. raylib's
// "pressed" edge is true for exactly one input poll, and the only poll is
// EndDrawing (nccu::engine::render::DrawScope's dtor). When one screen returns on an Enter
// press and the next screen starts before the next EndDrawing, the new
// screen sees the very same Enter edge — so a single physical press would
// drive two transitions: opening 遊戲說明 would instantly close it again,
// confirming 開始遊戲 would also auto-pick the first persona, and so on.
//
// PressLatch suppresses a press until the key has been observed RELEASED
// at least once since the latch armed. A screen keeps one latch per
// confirm key; the press it inherited from the previous screen is ignored
// because that key is still held down when the new screen begins. Pure
// logic, no raylib — unit-testable headless.
class PressLatch {
public:
    // Feed once per frame with the key's level (down) and edge (pressed).
    // Returns true exactly on the first fresh press observed after the key
    // has been released since the latch last armed.
    bool Fired(bool down, bool pressed) noexcept {
        if (!down) armed_ = true;            // released → ready for next press
        if (armed_ && pressed) {
            armed_ = false;                  // consume; require a new release
            return true;
        }
        return false;
    }

private:
    bool armed_ = false;   // true only after the key has been seen released
};

} // namespace nccu

#endif // UI_PRESS_LATCH_H_
