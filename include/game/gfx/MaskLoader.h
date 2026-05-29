#ifndef GFX_MASK_LOADER_H_
#define GFX_MASK_LOADER_H_
#include "engine/render/ImageDecoder.h"
#include "game/world/CollisionMask.h"
#include <cstdio>
#include <string>

namespace nccu::game::gfx {

// Loads the terrain walkability PNG into a raylib-free CollisionMask.
// Blueprint Phase 4 R5: the raylib::Image lifetime + decode are
// confined to engine/render/ImageDecoder.cpp; this header pulls in
// zero raylib symbols. A pixel is WALKABLE iff it is fully transparent
// or pure white; everything else is SOLID. That dual rule survives both
// an RGB white-background export and an RGBA transparent-background
// export, so the artist can flatten the file however their editor
// likes without silently turning the whole world solid.
//
// `primary` is the hand-authored mask; if it is missing the
// tool-generated `fallback` (buildings + river only, no props) is used
// so a fresh checkout still collides correctly. If both are missing the
// mask is empty (everything walkable) and one line is logged — terrain
// collision degrades visibly instead of failing silently.
inline CollisionMask LoadCollisionMask(const std::string& primary,
                                       const std::string& fallback) {
    nccu::engine::render::DecodedImage img = nccu::engine::render::LoadRgba8Image(primary);
    const char* used = primary.c_str();
    if (img.Empty()) {
        img  = nccu::engine::render::LoadRgba8Image(fallback);
        used = fallback.c_str();
    }
    if (img.Empty()) {
        std::fprintf(stderr,
            "[CollisionMask] WARNING: neither '%s' nor '%s' found — "
            "terrain collision DISABLED (everything walkable)\n",
            primary.c_str(), fallback.c_str());
        return CollisionMask{};
    }

    const int w = img.width;
    const int h = img.height;
    const auto& px = img.rgba8;
    std::vector<std::uint8_t> solid(
        static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (std::size_t i = 0, n = solid.size(); i < n; ++i) {
        const unsigned char r = px[i * 4 + 0];
        const unsigned char g = px[i * 4 + 1];
        const unsigned char b = px[i * 4 + 2];
        const unsigned char a = px[i * 4 + 3];
        const bool walkable =
            (a == 0) || (r == 255 && g == 255 && b == 255);
        solid[i] = walkable ? 0 : 1;
    }
    std::fprintf(stderr, "[CollisionMask] loaded %dx%d from '%s'\n",
                 w, h, used);
    return CollisionMask{w, h, std::move(solid)};
}

} // namespace nccu::game::gfx

#endif // GFX_MASK_LOADER_H_
