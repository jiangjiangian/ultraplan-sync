#ifndef GFX_RENDERER_H_
#define GFX_RENDERER_H_
#include "raylib.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/render/Texture.h"
#include "engine/math/Vec2.h"

namespace nccu::engine::render {

/**
 * @file Renderer.h
 * @brief 基本繪圖包裝：以引擎型別呼叫 raylib 的矩形／像素／材質繪製，並提供流暢介面。
 */

/**
 * @brief raylib 即時繪圖呼叫的薄包裝，提供可串接的流暢介面。
 *
 * 把 ::ClearBackground／::DrawRectangleRec／::DrawTextureV 等呼叫收斂在此，使上層
 * 以引擎的 Color／Rect／Vec2 型別作畫，raylib 細節不外漏。每個方法回傳 *this 以便
 * 在同一幀內串接。
 */
class Renderer {
public:
    /** @brief 以指定顏色清除背景。@param[in] c 清除色。@return *this。 */
    Renderer& Clear(nccu::engine::math::Color c) noexcept {
        ::ClearBackground(::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    /**
     * @brief 繪製填滿矩形。
     * @param[in] r 矩形範圍。
     * @param[in] c 填色。
     * @return *this。
     *
     * 類別內方法名 Rect 會遮蔽命名空間型別 nccu::engine::math::Rect，故參數列以詳述
     * 形式 struct nccu::engine::math::Rect 指明型別。
     */
    Renderer& Rect(struct nccu::engine::math::Rect r, nccu::engine::math::Color c) noexcept {
        ::DrawRectangleRec(::Rectangle{r.x, r.y, r.width, r.height},
                           ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    /**
     * @brief 繪製矩形外框。
     * @param[in] r         矩形範圍。
     * @param[in] c         線色。
     * @param[in] thickness 線寬，預設 1.0f。
     * @return *this。
     */
    Renderer& RectLines(struct nccu::engine::math::Rect r, nccu::engine::math::Color c, float thickness = 1.0f) noexcept {
        ::DrawRectangleLinesEx(::Rectangle{r.x, r.y, r.width, r.height},
                               thickness,
                               ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    /**
     * @brief 繪製單一像素點。
     * @param[in] p 像素座標。
     * @param[in] c 顏色。
     * @return *this。
     */
    Renderer& Pixel(nccu::engine::math::Vec2 p, nccu::engine::math::Color c) noexcept {
        ::DrawPixelV(::Vector2{p.x, p.y}, ::Color{c.r, c.g, c.b, c.a});
        return *this;
    }

    /**
     * @brief 於指定位置原尺寸繪製整張材質。
     * @param[in] tex  材質。
     * @param[in] pos  左上角座標。
     * @param[in] tint 著色，預設白（不改色）。
     * @return *this。
     *
     * 類別範圍內方法名 Texture 會遮蔽型別 nccu::engine::render::Texture，故參數列以
     * 詳述形式 class Texture 指明型別（同上方 Rect 的處理）。
     */
    Renderer& Texture(const class Texture& tex,
                      nccu::engine::math::Vec2 pos,
                      nccu::engine::math::Color tint = nccu::engine::math::Colors::White) noexcept {
        ::DrawTextureV(tex.Raw(),
                       ::Vector2{pos.x, pos.y},
                       ::Color{tint.r, tint.g, tint.b, tint.a});
        return *this;
    }

    /**
     * @brief 將材質的來源子矩形繪製到螢幕上的目標矩形。
     * @param[in] tex  材質。
     * @param[in] src  材質上的來源子矩形。
     * @param[in] dest 螢幕上的目標矩形。
     * @param[in] tint 著色，預設白（不改色）。
     * @return *this。
     *
     * 當 dest 與 src 尺寸不同時 sprite 會被縮放——供 sprite sheet 切幀，以及選角格
     * 放大預覽之用。
     */
    Renderer& TextureRect(const class Texture& tex,
                          struct nccu::engine::math::Rect src,
                          struct nccu::engine::math::Rect dest,
                          nccu::engine::math::Color tint = nccu::engine::math::Colors::White) noexcept {
        ::DrawTexturePro(tex.Raw(),
                         ::Rectangle{src.x,  src.y,  src.width,  src.height},
                         ::Rectangle{dest.x, dest.y, dest.width, dest.height},
                         ::Vector2{0.0f, 0.0f}, 0.0f,
                         ::Color{tint.r, tint.g, tint.b, tint.a});
        return *this;
    }
};

} // namespace nccu::engine::render

#endif // GFX_RENDERER_H_
