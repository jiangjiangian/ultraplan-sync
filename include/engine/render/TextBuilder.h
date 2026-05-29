#ifndef GFX_TEXT_BUILDER_H_
#define GFX_TEXT_BUILDER_H_
#include "raylib.h"
#include "engine/math/Color.h"
#include "engine/render/Font.h"
#include "engine/math/Vec2.h"
#include <string>
#include <utility>

namespace nccu::gfx {

class TextBuilder {
public:
    explicit TextBuilder(std::string text) : text_(std::move(text)) {}

    TextBuilder& At(nccu::engine::math::Vec2 p)            noexcept { position_ = p; return *this; }
    TextBuilder& Size(int s)           noexcept { size_ = s; return *this; }
    TextBuilder& Color(struct nccu::engine::math::Color c) noexcept { color_ = c; return *this; }

    void Draw() const {
        const ::Color rc{color_.r, color_.g, color_.b, color_.a};
        // The default font is ASCII-only; route through the loaded CJK
        // font (DrawTextEx) whenever it is available so Chinese renders.
        // Spacing follows raylib's own DrawText convention (size/10).
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

    // Exact rendered size of this text at the current font/size, so a
    // backing panel can hug it precisely instead of guessing from a glyph
    // count. Uses the loaded CJK font when present (the same path Draw()
    // takes), else raylib's default-font measure; both honour the size/10
    // spacing Draw() uses. Falls back to a width-only estimate if no font
    // /GL context exists (headless tests never call this on a live panel).
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

    nccu::engine::math::Vec2                GetPosition() const noexcept { return position_; }
    int                 GetSize()     const noexcept { return size_; }
    struct nccu::engine::math::Color        GetColor()    const noexcept { return color_; }

private:
    std::string         text_;
    nccu::engine::math::Vec2                position_{0.0f, 0.0f};
    int                 size_{10};
    struct nccu::engine::math::Color        color_{0, 0, 0, 255};
};

} // namespace nccu::gfx

#endif // GFX_TEXT_BUILDER_H_
