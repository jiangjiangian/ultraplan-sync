#ifndef GFX_I_RENDERER_H_
#define GFX_I_RENDERER_H_
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <string_view>

namespace nccu::engine::render {

class Texture; // 前向宣告——raylib 不得滲入此標頭

/**
 * @file IRenderer.h
 * @brief 抽象繪圖服務介面：所有 raylib 繪圖呼叫的隔離邊界，使 Model 不直接相依 raylib。
 */

/**
 * @brief 由 View 層交給 Model 類別使用的抽象繪圖服務。
 *
 * 每個 raylib 繪圖呼叫都藏在此介面後，因此寫入它的 Model 子類別不會拉進 raylib
 * 系統標頭（維持 UI↔資料解耦）。RaylibRenderer 為具體實作；測試可用 spy／mock
 * 在沒有 GL context 的情況下驗證繪圖呼叫。
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;
    /**
     * @brief 繪製填滿矩形。
     * @param[in] r 矩形範圍。
     * @param[in] c 填色。
     */
    virtual void DrawRect(nccu::engine::math::Rect r, nccu::engine::math::Color c) = 0;
    /**
     * @brief 繪製 sprite（材質的子矩形縮放貼到目標矩形）。
     * @param[in] tex  來源材質。
     * @param[in] src  材質上的來源子矩形。
     * @param[in] dest 螢幕上的目標矩形。
     * @param[in] tint 著色，預設白（不改色）。
     */
    virtual void DrawSprite(const Texture& tex, nccu::engine::math::Rect src, nccu::engine::math::Rect dest,
                            nccu::engine::math::Color tint = nccu::engine::math::Colors::White) = 0;
    /**
     * @brief 繪製文字。
     * @param[in] text 文字內容。
     * @param[in] pos  左上角座標。
     * @param[in] size 字級。
     * @param[in] c    文字顏色。
     */
    virtual void DrawText(std::string_view text, nccu::engine::math::Vec2 pos, int size,
                          nccu::engine::math::Color c) = 0;
};

} // namespace nccu::engine::render

#endif // GFX_I_RENDERER_H_
