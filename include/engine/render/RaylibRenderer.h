#ifndef GFX_RAYLIB_RENDERER_H_
#define GFX_RAYLIB_RENDERER_H_
#include "engine/render/IRenderer.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/render/Texture.h"
#include <string>

namespace nccu::gfx {

// Concrete IRenderer adapter. The only place IRenderer calls cross into
// raylib — delegates straight to the existing nccu::gfx wrappers, which
// own every ::Draw* call. Stateless and cheap to construct per draw.
class RaylibRenderer final : public IRenderer {
public:
    void DrawRect(Rect r, Color c) override {
        Renderer{}.Rect(r, c);
    }

    void DrawSprite(const Texture& tex, Rect src, Rect dest,
                    Color tint = Colors::White) override {
        Renderer{}.TextureRect(tex, src, dest, tint);
    }

    void DrawText(std::string_view text, Vec2 pos, int size,
                  Color c) override {
        TextBuilder{std::string{text}}.At(pos).Size(size).Color(c).Draw();
    }
};

} // namespace nccu::gfx

#endif // GFX_RAYLIB_RENDERER_H_
