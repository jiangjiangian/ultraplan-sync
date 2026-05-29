#ifndef GFX_RECT_H_
#define GFX_RECT_H_
#include "engine/math/Vec2.h"   // Contains() 以 Vec2 表示點

/**
 * @file Rect.h
 * @brief 軸對齊矩形（AABB）：碰撞盒與點／矩形包含測試的基礎型別。
 */

namespace nccu::engine::math {

/**
 * @brief 以左上角座標與寬高描述的軸對齊矩形（AABB）。
 *
 * 全專案碰撞偵測的共同幾何；x/y 為左上角，width/height 向右下延伸。
 */
struct Rect {
    float x{0.0f};        ///< 左上角 x
    float y{0.0f};        ///< 左上角 y
    float width{0.0f};    ///< 寬（向右）
    float height{0.0f};   ///< 高（向下）

    /**
     * @brief 測試點是否落在矩形內。
     * @param[in] p 待測點。
     * @return 點在矩形內回傳 true；右／下邊界採半開區間（不含），避免相鄰矩形重複命中。
     */
    constexpr bool Contains(Vec2 p) const noexcept {
        return p.x >= x && p.x < x + width
            && p.y >= y && p.y < y + height;
    }

    /**
     * @brief 測試兩矩形是否相交（AABB 重疊）。
     * @param[in] o 另一矩形。
     * @return 兩者有重疊區域回傳 true。
     */
    constexpr bool Intersects(Rect o) const noexcept {
        return !(o.x >= x + width
              || o.x + o.width <= x
              || o.y >= y + height
              || o.y + o.height <= y);
    }
};

} // namespace nccu::engine::math

#endif // GFX_RECT_H_
