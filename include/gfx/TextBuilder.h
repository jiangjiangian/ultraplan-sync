#ifndef GFX_TEXT_BUILDER_H_
#define GFX_TEXT_BUILDER_H_
#include "raylib.h"
#include "gfx/Color.h"
#include "gfx/Font.h"
#include "gfx/Vec2.h"
#include <string>
#include <utility>

namespace nccu::gfx {

class TextBuilder {
public:
    explicit TextBuilder(std::string text) : text_(std::move(text)) {}

    TextBuilder& At(Vec2 p)            noexcept { position_ = p; return *this; }
    TextBuilder& Size(int s)           noexcept { size_ = s; return *this; }
    TextBuilder& Color(struct Color c) noexcept { color_ = c; return *this; }

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

    Vec2                GetPosition() const noexcept { return position_; }
    int                 GetSize()     const noexcept { return size_; }
    struct Color        GetColor()    const noexcept { return color_; }

private:
    std::string         text_;
    Vec2                position_{0.0f, 0.0f};
    int                 size_{10};
    struct Color        color_{0, 0, 0, 255};
};

} // namespace nccu::gfx

#endif // GFX_TEXT_BUILDER_H_
