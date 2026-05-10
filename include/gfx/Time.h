#pragma once
#include "raylib.h"

namespace nccu::gfx {

struct Time {
    static float DeltaSeconds() noexcept { return ::GetFrameTime(); }
    static int   FpsAvg()       noexcept { return ::GetFPS(); }
};

} // namespace nccu::gfx
