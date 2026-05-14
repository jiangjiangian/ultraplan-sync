#ifndef GFX_TIME_H_
#define GFX_TIME_H_
#include "raylib.h"

namespace nccu::gfx {

struct Time {
    static float DeltaSeconds() noexcept { return ::GetFrameTime(); }
    static int   FpsAvg()       noexcept { return ::GetFPS(); }
};

} // namespace nccu::gfx

#endif // GFX_TIME_H_
