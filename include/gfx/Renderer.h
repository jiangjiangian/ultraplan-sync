#pragma once
#include "raylib.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"
#include "gfx/Texture.h"
#include "gfx/Vec2.h"

namespace nccu::gfx {

class Renderer {
public:
    Renderer& Clear(Color c) noexcept {
        ::ClearBackground(::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    // Inside the class, the method name `Rect` shadows the namespace type
    // `nccu::gfx::Rect`, so we use the elaborated form `struct Rect` to
    // refer to the type in the parameter list.
    Renderer& Rect(struct Rect r, Color c) noexcept {
        ::DrawRectangleRec(::Rectangle{r.x, r.y, r.width, r.height},
                           ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    Renderer& RectLines(struct Rect r, Color c, float thickness = 1.0f) noexcept {
        ::DrawRectangleLinesEx(::Rectangle{r.x, r.y, r.width, r.height},
                               thickness,
                               ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    Renderer& Pixel(Vec2 p, Color c) noexcept {
        ::DrawPixelV(::Vector2{p.x, p.y}, ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    // Method name `Texture` shadows the type nccu::gfx::Texture inside the
    // class scope, so we use the elaborated form `class Texture` in the
    // parameter list (mirrors the Rect pattern above).
    Renderer& Texture(const class Texture& tex,
                      Vec2 pos,
                      Color tint = Colors::White) noexcept {
        ::DrawTextureV(tex.Raw(),
                       ::Vector2{pos.x, pos.y},
                       ::Color{tint.r, tint.g, tint.b, tint.a});
        return *this;
    }
};

} // namespace nccu::gfx
