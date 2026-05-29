#ifndef GFX_RAYLIB_RENDERER_H_
#define GFX_RAYLIB_RENDERER_H_
#include "engine/render/IRenderer.h"
#include "engine/render/Renderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/render/Texture.h"
#include <string>

namespace nccu::engine::render {

/**
 * @file RaylibRenderer.h
 * @brief IRenderer 的具體 raylib 實作：IRenderer 呼叫越界進入 raylib 的唯一處。
 */

/**
 * @brief 將 IRenderer 介面轉接到 raylib 繪圖的具體實作。
 *
 * 為 IRenderer 呼叫進入 raylib 的唯一位置——直接委派給既有的渲染包裝（Renderer／
 * TextBuilder，由它們擁有每個 ::Draw* 呼叫）。無狀態，可在每次繪製時廉價建構。
 */
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

} // namespace nccu::engine::render

#endif // GFX_RAYLIB_RENDERER_H_
