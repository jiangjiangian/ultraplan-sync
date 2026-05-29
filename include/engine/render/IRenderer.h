#ifndef GFX_I_RENDERER_H_
#define GFX_I_RENDERER_H_
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <string_view>

namespace nccu::gfx {

class Texture; // forward decl — raylib must NOT leak into this header

// Abstract draw service handed to Model classes by the View layer.
// Every raylib call lives behind this interface, so a Model subclass
// writing to it never pulls the raylib system header. RaylibRenderer
// is the concrete implementation; tests can use a spy/mock to assert
// draw calls without a GL context.
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) = 0;
    virtual void DrawSprite(const Texture& tex, nccu::engine::math::Rect src, nccu::engine::math::Rect dest,
                            nccu::engine::math::Color tint = nccu::engine::math::Colors::White) = 0;
    virtual void DrawText(std::string_view text, nccu::engine::math::Vec2 pos, int size,
                          nccu::engine::math::Color c) = 0;
};

} // namespace nccu::gfx

#endif // GFX_I_RENDERER_H_
