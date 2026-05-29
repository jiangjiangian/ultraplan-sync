#ifndef GFX_CAMERA2_D_H_
#define GFX_CAMERA2_D_H_
#include "engine/math/Vec2.h"

namespace nccu::engine::render {

/**
 * @file Camera2D.h
 * @brief raylib ::Camera2D 的轉接器：以引擎型別描述 2D 攝影機，並提供流暢設定介面。
 */

/**
 * @brief 2D 攝影機參數（::Camera2D 的轉接型別）。
 *
 * 欄位名比照 raylib，使對映一目了然；流暢設定器支援單行串接：
 *     cam.Follow(playerPos, screenCenter).WithZoom(1.0f);
 */
struct Camera2D {
    nccu::engine::math::Vec2  offset{0.0f, 0.0f};   ///< 螢幕上的偏移（通常為畫面中心）
    nccu::engine::math::Vec2  target{0.0f, 0.0f};   ///< 攝影機對準的世界座標
    float rotation{0.0f};   ///< 旋轉角度
    float zoom{1.0f};       ///< 縮放倍率

    /**
     * @brief 讓攝影機追隨某世界座標並對齊到螢幕中心。
     * @param[in] worldTarget  要對準的世界座標。
     * @param[in] screenCenter 對應的螢幕中心偏移。
     * @return *this，便於鏈式呼叫。
     */
    Camera2D& Follow(nccu::engine::math::Vec2 worldTarget, nccu::engine::math::Vec2 screenCenter) noexcept {
        target = worldTarget;
        offset = screenCenter;
        return *this;
    }
    /** @brief 設定縮放倍率。@param[in] z 縮放值。@return *this。 */
    Camera2D& WithZoom(float z) noexcept     { zoom = z;     return *this; }
    /** @brief 設定旋轉角度。@param[in] r 角度。@return *this。 */
    Camera2D& WithRotation(float r) noexcept { rotation = r; return *this; }

    /**
     * @brief 夾住 target，使以 target 為中心、大小為 viewportSize 的視口落在 [0, worldSize] 內。
     * @param[in] worldSize    世界尺寸。
     * @param[in] viewportSize 視口尺寸。
     * @return *this，便於鏈式呼叫。
     *
     * 若某軸上世界比視口還小，則該軸的 target 釘在世界中點（避免露出世界外的空白）。
     */
    Camera2D& ClampToWorld(nccu::engine::math::Vec2 worldSize, nccu::engine::math::Vec2 viewportSize) noexcept {
        auto clampAxis = [](float v, float half, float worldExtent) noexcept {
            if (worldExtent < 2.0f * half) return worldExtent * 0.5f;
            if (v < half)                  return half;
            if (v > worldExtent - half)    return worldExtent - half;
            return v;
        };
        target.x = clampAxis(target.x, viewportSize.x * 0.5f, worldSize.x);
        target.y = clampAxis(target.y, viewportSize.y * 0.5f, worldSize.y);
        return *this;
    }
};

} // namespace nccu::engine::render

#endif // GFX_CAMERA2_D_H_
