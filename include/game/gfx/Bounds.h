#ifndef GFX_BOUNDS_H_
#define GFX_BOUNDS_H_
#include "engine/math/Vec2.h"

/**
 * @file Bounds.h
 * @brief 把實體鉗制在世界邊界內的純幾何工具（無 raylib、無模擬狀態）。
 */
namespace nccu::game::gfx {

/**
 * @brief 鉗制 `pos`，使錨定於該點、大小為 `size` 的 AABB 完整落在
 *        [0, worldSize] 範圍內。
 * @param pos       AABB 的錨點（左上角）世界座標。
 * @param size      AABB 的寬高。
 * @param worldSize 世界邊界尺寸。
 * @return 鉗制後的錨點。
 *
 * 某軸上 `size` 大於 `worldSize` 時（AABB 比世界還寬），該軸釘在 0——
 * 無有效區間可鉗，取下界即可，避免回傳負座標。
 */
inline nccu::engine::math::Vec2 ClampToWorld(nccu::engine::math::Vec2 pos, nccu::engine::math::Vec2 size, nccu::engine::math::Vec2 worldSize) noexcept {
    auto clamp1 = [](float v, float lo, float hi) noexcept {
        if (hi < lo) return lo;   // 此軸 size > world：無有效區間，釘 0
        if (v < lo)  return lo;
        if (v > hi)  return hi;
        return v;
    };
    return nccu::engine::math::Vec2{
        clamp1(pos.x, 0.0f, worldSize.x - size.x),
        clamp1(pos.y, 0.0f, worldSize.y - size.y)
    };
}

} // namespace nccu::game::gfx

#endif // GFX_BOUNDS_H_
