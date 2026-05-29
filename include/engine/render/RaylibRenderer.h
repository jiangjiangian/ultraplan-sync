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
    void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) override {
        Renderer{}.Rect(r, c);
    }

    void DrawSprite(const Texture& tex, nccu::engine::math::Rect src, nccu::engine::math::Rect dest,
                    nccu::engine::math::Color tint = nccu::engine::math::Colors::White) override {
        Renderer{}.TextureRect(tex, src, dest, tint);
    }

    void DrawText(std::string_view text, nccu::engine::math::Vec2 pos, int size,
                  nccu::engine::math::Color c) override {
        TextBuilder{std::string{text}}.At(pos).Size(size).Color(c).Draw();
    }
};

} // namespace nccu::gfx

#endif // GFX_RAYLIB_RENDERER_H_
