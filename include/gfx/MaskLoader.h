#ifndef GFX_MASK_LOADER_H_
#define GFX_MASK_LOADER_H_
#include "raylib.h"
#include "CollisionMask.h"
#include <cstdio>
#include <string>

namespace nccu::gfx {

// Loads the terrain walkability PNG into a raylib-free CollisionMask.
// raylib's CPU Image API lives ONLY here so the rest of the engine
// never sees raylib.h. A pixel is WALKABLE iff it is fully transparent
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
    ::Image img = ::LoadImage(primary.c_str());
    const char* used = primary.c_str();
    if (img.data == nullptr || img.width <= 0) {
        if (img.data) ::UnloadImage(img);
        img = ::LoadImage(fallback.c_str());
        used = fallback.c_str();
    }
    if (img.data == nullptr || img.width <= 0) {
        if (img.data) ::UnloadImage(img);
        std::fprintf(stderr,
            "[CollisionMask] WARNING: neither '%s' nor '%s' found — "
            "terrain collision DISABLED (everything walkable)\n",
            primary.c_str(), fallback.c_str());
        return CollisionMask{};
    }

    ::ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const int w = img.width, h = img.height;
    const auto* px = static_cast<const unsigned char*>(img.data);
    std::vector<std::uint8_t> solid(static_cast<std::size_t>(w) * h);
    for (std::size_t i = 0, n = solid.size(); i < n; ++i) {
        const unsigned char r = px[i * 4 + 0];
        const unsigned char g = px[i * 4 + 1];
        const unsigned char b = px[i * 4 + 2];
        const unsigned char a = px[i * 4 + 3];
        const bool walkable =
            (a == 0) || (r == 255 && g == 255 && b == 255);
        solid[i] = walkable ? 0 : 1;
    }
    ::UnloadImage(img);
    std::fprintf(stderr, "[CollisionMask] loaded %dx%d from '%s'\n",
                 w, h, used);
    return CollisionMask{w, h, std::move(solid)};
}

} // namespace nccu::gfx

#endif // GFX_MASK_LOADER_H_
