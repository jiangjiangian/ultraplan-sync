#ifndef GFX_KEY_H_
#define GFX_KEY_H_
#include "raylib.h"

namespace nccu::gfx {

enum class Key : int {
    A = KEY_A, B = KEY_B, C = KEY_C, D = KEY_D, E = KEY_E,
    F = KEY_F, G = KEY_G, H = KEY_H, I = KEY_I, J = KEY_J,
    K = KEY_K, L = KEY_L, M = KEY_M, N = KEY_N, O = KEY_O,
    P = KEY_P, Q = KEY_Q, R = KEY_R, S = KEY_S, T = KEY_T,
    U = KEY_U, V = KEY_V, W = KEY_W, X = KEY_X, Y = KEY_Y,
    Z = KEY_Z,
    Space  = KEY_SPACE,
    Enter  = KEY_ENTER,
    Escape = KEY_ESCAPE,
    Up     = KEY_UP,
    Down   = KEY_DOWN,
    Left   = KEY_LEFT,
    Right  = KEY_RIGHT,
};

constexpr int ToRaylibKey(Key k) noexcept { return static_cast<int>(k); }

} // namespace nccu::gfx

#endif // GFX_KEY_H_
