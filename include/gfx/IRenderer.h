#ifndef GFX_I_RENDERER_H_
#define GFX_I_RENDERER_H_
#include "gfx/Color.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
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
    virtual void DrawRect(Rect r, Color c) = 0;
    virtual void DrawSprite(const Texture& tex, Rect src, Rect dest,
                            Color tint = Colors::White) = 0;
    virtual void DrawText(std::string_view text, Vec2 pos, int size,
                          Color c) = 0;
};

} // namespace nccu::gfx

#endif // GFX_I_RENDERER_H_
