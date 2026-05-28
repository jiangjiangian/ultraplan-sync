#ifndef GFX_RECT_H_
#define GFX_RECT_H_
#include "engine/math/Vec2.h"

namespace nccu::gfx {

struct Rect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    constexpr bool Contains(Vec2 p) const noexcept {
        return p.x >= x && p.x < x + width
            && p.y >= y && p.y < y + height;
    }

    constexpr bool Intersects(Rect o) const noexcept {
        return !(o.x >= x + width
              || o.x + o.width <= x
              || o.y >= y + height
              || o.y + o.height <= y);
    }
};

} // namespace nccu::gfx

#endif // GFX_RECT_H_
