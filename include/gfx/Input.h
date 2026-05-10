#pragma once
#include "raylib.h"
#include "gfx/Key.h"

namespace nccu::gfx {

struct Input {
    static bool IsDown(Key k) noexcept     { return ::IsKeyDown(ToRaylibKey(k)); }
    static bool IsPressed(Key k) noexcept  { return ::IsKeyPressed(ToRaylibKey(k)); }
    static bool IsReleased(Key k) noexcept { return ::IsKeyReleased(ToRaylibKey(k)); }
};

} // namespace nccu::gfx
