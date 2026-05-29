#ifndef GFX_DRAW_SCOPE_H_
#define GFX_DRAW_SCOPE_H_
#include "raylib.h"

namespace nccu::engine::render {

class DrawScope {
public:
    DrawScope() noexcept { ::BeginDrawing(); }
    ~DrawScope() { ::EndDrawing(); }

    DrawScope(const DrawScope&)            = delete;
    DrawScope& operator=(const DrawScope&) = delete;
    DrawScope(DrawScope&&)                 = delete;
    DrawScope& operator=(DrawScope&&)      = delete;
};

} // namespace nccu::engine::render

#endif // GFX_DRAW_SCOPE_H_
