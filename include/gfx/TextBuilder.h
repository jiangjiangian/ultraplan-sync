#ifndef GFX_TEXT_BUILDER_H_
#define GFX_TEXT_BUILDER_H_
#include "raylib.h"
#include "gfx/Color.h"
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
        ::DrawText(text_.c_str(),
                   static_cast<int>(position_.x),
                   static_cast<int>(position_.y),
                   size_,
                   ::Color{color_.r, color_.g, color_.b, color_.a});
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
