#ifndef GFX_KEY_H_
#define GFX_KEY_H_
#include "raylib.h"

/**
 * @file Key.h
 * @brief 型別安全的按鍵列舉：把引擎用到的按鍵對映到 raylib 的 KEY_* 整數值。
 */

namespace nccu::engine::input {

/**
 * @brief 引擎使用的按鍵列舉，列舉值即對應的 raylib KEY_* 整數。
 *
 * 以底層為 int 的 enum class 包住 raylib 鍵碼，讓上層以具名常數操作，避免裸整數
 * 鍵碼散落各處；透過 ToRaylibKey() 還原為 raylib 期望的整數。
 */
enum class Key : int {
    A = KEY_A, B = KEY_B, C = KEY_C, D = KEY_D, E = KEY_E,
    F = KEY_F, G = KEY_G, H = KEY_H, I = KEY_I, J = KEY_J,
    K = KEY_K, L = KEY_L, M = KEY_M, N = KEY_N, O = KEY_O,
    P = KEY_P, Q = KEY_Q, R = KEY_R, S = KEY_S, T = KEY_T,
    U = KEY_U, V = KEY_V, W = KEY_W, X = KEY_X, Y = KEY_Y,
    Z = KEY_Z,
    Space     = KEY_SPACE,
    Tab       = KEY_TAB,
    Enter     = KEY_ENTER,
    Escape    = KEY_ESCAPE,
    Backspace = KEY_BACKSPACE,
    Up        = KEY_UP,
    Down      = KEY_DOWN,
    Left      = KEY_LEFT,
    Right     = KEY_RIGHT,
};

/** @brief 將 Key 轉回 raylib 期望的整數鍵碼。@param[in] k 按鍵列舉值。@return 對應的 raylib 鍵碼。 */
constexpr int ToRaylibKey(Key k) noexcept { return static_cast<int>(k); }

} // namespace nccu::engine::input

#endif // GFX_KEY_H_
