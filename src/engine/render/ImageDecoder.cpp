#include "engine/render/ImageDecoder.h"

#include "raylib.h"

#include <cstddef>

namespace nccu::gfx {

DecodedImage LoadRgba8Image(const std::string& path) {
    ::Image img = ::LoadImage(path.c_str());
    if (img.data == nullptr || img.width <= 0) {
        if (img.data) ::UnloadImage(img);
        return DecodedImage{};
    }
    ::ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    DecodedImage out;
    out.width  = img.width;
    out.height = img.height;
    const auto pixelCount =
        static_cast<std::size_t>(out.width) *
        static_cast<std::size_t>(out.height) * 4u;
    const auto* src = static_cast<const std::uint8_t*>(img.data);
    out.rgba8.assign(src, src + pixelCount);

    ::UnloadImage(img);
    return out;
}

} // namespace nccu::gfx
