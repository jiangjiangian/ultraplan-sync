#ifndef GFX_MASK_LOADER_H_
#define GFX_MASK_LOADER_H_
#include "engine/render/ImageDecoder.h"
#include "game/world/CollisionMask.h"
#include <cstdio>
#include <string>

namespace nccu::game::gfx {

/**
 * @brief 把地形可行走 PNG 載入為不依賴 raylib 的 CollisionMask。
 * @param primary  手繪遮罩的檔案路徑。
 * @param fallback primary 缺檔時改用的工具生成遮罩（僅建築 + 河流，無雜物）。
 * @return 解析後的碰撞遮罩；兩個檔案皆缺時為空遮罩（全部可行走）。
 *
 * raylib::Image 的生命週期與解碼全部封在 engine/render/ImageDecoder.cpp，本標頭
 * 不引入任何 raylib 符號。可行走判定：完全透明「或」純白才可行走，其餘皆為實心。
 * 這套雙重規則同時容納「RGB 白底匯出」與「RGBA 透明底匯出」兩種來源，讓美術不論
 * 在編輯器裡如何壓平圖層，都不會把整個世界悄悄變成實心。
 *
 * primary 缺檔才退回 fallback，使全新 checkout 仍能正確碰撞；兩者皆缺時遮罩為空
 * 並輸出一行警告——讓地形碰撞「明顯」降級，而非無聲失效。
 */
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
