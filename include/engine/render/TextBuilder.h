#ifndef GFX_TEXT_BUILDER_H_
#define GFX_TEXT_BUILDER_H_
#include "raylib.h"
#include "engine/math/Color.h"
#include "engine/render/Font.h"
#include "engine/math/Vec2.h"
#include <string>
#include <utility>

namespace nccu::engine::render {

/**
 * @file TextBuilder.h
 * @brief 文字繪製建構器：以流暢介面設定文字內容／位置／字級／顏色，並在可用時走 CJK 字型。
 */

/**
 * @brief 以建構器模式組裝並繪製一段文字。
 *
 * 透過 At／Size／Color 流暢設定後呼叫 Draw() 繪製，或以 Measure() 量測實際版面尺寸。
 * 當 CJK 字型已載入時改走 DrawTextEx 以正確顯示中文，否則退回 raylib 預設字型。
 */
class TextBuilder {
public:
    /** @brief 以文字內容建構。@param[in] text 要繪製的文字。 */
    explicit TextBuilder(std::string text) : text_(std::move(text)) {}

    /** @brief 設定繪製位置。@param[in] p 左上角座標。@return *this。 */
    TextBuilder& At(nccu::engine::math::Vec2 p)            noexcept { position_ = p; return *this; }
    /** @brief 設定字級。@param[in] s 字級。@return *this。 */
    TextBuilder& Size(int s)           noexcept { size_ = s; return *this; }
    /** @brief 設定文字顏色。@param[in] c 顏色。@return *this。 */
    TextBuilder& Color(struct nccu::engine::math::Color c) noexcept { color_ = c; return *this; }

    /** @brief 將文字繪製到畫面上。 */
    void Draw() const {
        const ::Color rc{color_.r, color_.g, color_.b, color_.a};
        // 預設字型僅含 ASCII；只要 CJK 字型已載入便改走 DrawTextEx，使中文得以顯示。
        // 字距沿用 raylib DrawText 自身慣例（size/10）。
        if (IsCJKFontLoaded()) {
            ::DrawTextEx(CJKFont(), text_.c_str(),
                         ::Vector2{position_.x, position_.y},
                         static_cast<float>(size_),
                         static_cast<float>(size_) / 10.0f,
                         rc);
        } else {
            ::DrawText(text_.c_str(),
                       static_cast<int>(position_.x),
                       static_cast<int>(position_.y),
                       size_, rc);
        }
    }

    /**
     * @brief 量測此文字在當前字型／字級下的實際版面尺寸。
     * @return 文字的寬高（像素）。
     *
     * 讓背板可精準貼合文字，而非以字數估算。CJK 字型存在時走與 Draw() 相同路徑，
     * 否則走 raylib 預設字型量測；兩者皆套用 Draw() 所用的 size/10 字距。若無
     * 字型／GL context 則退回僅估寬度（無頭測試不會對實際面板呼叫此函式）。
     */
    nccu::engine::math::Vec2 Measure() const {
        const float spacing = static_cast<float>(size_) / 10.0f;
        if (IsCJKFontLoaded()) {
            const ::Vector2 m = ::MeasureTextEx(
                CJKFont(), text_.c_str(),
                static_cast<float>(size_), spacing);
            return nccu::engine::math::Vec2{m.x, m.y};
        }
        return nccu::engine::math::Vec2{static_cast<float>(::MeasureText(text_.c_str(), size_)),
                    static_cast<float>(size_)};
    }

    /** @brief 取得目前設定的繪製位置。 */
    nccu::engine::math::Vec2                GetPosition() const noexcept { return position_; }
    /** @brief 取得目前設定的字級。 */
    int                 GetSize()     const noexcept { return size_; }
    /** @brief 取得目前設定的文字顏色。 */
    struct nccu::engine::math::Color        GetColor()    const noexcept { return color_; }

private:
    std::string         text_;                          ///< 文字內容
    nccu::engine::math::Vec2                position_{0.0f, 0.0f};   ///< 左上角座標
    int                 size_{10};                      ///< 字級
    struct nccu::engine::math::Color        color_{0, 0, 0, 255};    ///< 文字顏色
};

} // namespace nccu::engine::render

#endif // GFX_TEXT_BUILDER_H_
