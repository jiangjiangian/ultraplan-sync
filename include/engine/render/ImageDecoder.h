#ifndef ENGINE_RENDER_IMAGE_DECODER_H_
#define ENGINE_RENDER_IMAGE_DECODER_H_
#include <cstdint>
#include <string>
#include <vector>

namespace nccu::gfx {

// Blueprint Phase 4 R5 — confine raylib.h to engine/. A pure-data
// decoded RGBA8 image, returned by LoadRgba8Image below. Game-domain
// callers (game/world/MaskLoader.h, others) consume the pixel buffer
// without ever including raylib.h.
struct DecodedImage {
    int                       width  = 0;
    int                       height = 0;
    std::vector<std::uint8_t> rgba8;   // size = width * height * 4

    [[nodiscard]] bool Empty() const noexcept {
        return width <= 0 || height <= 0 || rgba8.empty();
    }
};

// Load `path` as an RGBA8 pixel buffer. Returns an empty DecodedImage
// (Empty() == true) on missing-file / decode-failure — never throws.
// Callers warn / fall back as appropriate (CollisionMask's two-tier
// path uses this pattern). Implementation hides every raylib::Image
// + ::LoadImage + ::ImageFormat + ::UnloadImage call inside the .cpp
// so this header pulls in zero raylib symbols (R5 invariant).
[[nodiscard]] DecodedImage LoadRgba8Image(const std::string& path);

} // namespace nccu::gfx

#endif // ENGINE_RENDER_IMAGE_DECODER_H_
