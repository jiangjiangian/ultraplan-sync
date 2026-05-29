#ifndef GFX_TIME_H_
#define GFX_TIME_H_
#include "raylib.h"

namespace nccu::engine::platform {

struct Time {
    // Deterministic replay: the autoplay harness pins a fixed step so a
    // scripted run is reproducible regardless of software-GL frame
    // pacing. Unset (<= 0, the default) -> real raylib frame time, so
    // normal play is unchanged.
    static void  SetFixedStep(float s) noexcept { fixed_ = s; }
    static float DeltaSeconds() noexcept {
        return fixed_ > 0.0f ? fixed_ : ::GetFrameTime();
    }
    static int   FpsAvg() noexcept { return ::GetFPS(); }

private:
    inline static float fixed_ = 0.0f;
};

} // namespace nccu::engine::platform

#endif // GFX_TIME_H_
