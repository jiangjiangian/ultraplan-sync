#ifndef GFX_INPUT_H_
#define GFX_INPUT_H_
#include "raylib.h"
#include "gfx/Key.h"

namespace nccu::gfx {

// Polymorphic input provider. LiveInput (the default) reads the real
// raylib devices, so normal play is bit-for-bit unchanged. The autoplay
// harness installs a scripted source via Input::SetSource() so a recorded
// timeline can drive the game deterministically and headless. This is the
// single choke point Player / GameController / CharacterSelect already
// funnel every key read through — swapping the source needs no call-site
// changes.
class InputSource {
public:
    virtual ~InputSource() = default;
    virtual bool IsDown(Key k)     const noexcept = 0;
    virtual bool IsPressed(Key k)  const noexcept = 0;
    virtual bool IsReleased(Key k) const noexcept = 0;
};

class LiveInput final : public InputSource {
public:
    bool IsDown(Key k)     const noexcept override { return ::IsKeyDown(ToRaylibKey(k)); }
    bool IsPressed(Key k)  const noexcept override { return ::IsKeyPressed(ToRaylibKey(k)); }
    bool IsReleased(Key k) const noexcept override { return ::IsKeyReleased(ToRaylibKey(k)); }
};

struct Input {
    // Default = live raylib devices. The harness sets a ScriptInput for
    // the run and restores nullptr (-> live) on teardown. Input never
    // owns the source; the harness keeps it alive for the whole session.
    static InputSource* Source() noexcept {
        static LiveInput live;
        return current_ ? current_ : &live;
    }
    static void SetSource(InputSource* s) noexcept { current_ = s; }

    static bool IsDown(Key k) noexcept     { return Source()->IsDown(k); }
    static bool IsPressed(Key k) noexcept  { return Source()->IsPressed(k); }
    static bool IsReleased(Key k) noexcept { return Source()->IsReleased(k); }

private:
    inline static InputSource* current_ = nullptr;
};

} // namespace nccu::gfx

#endif // GFX_INPUT_H_
