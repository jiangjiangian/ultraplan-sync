#include "engine/render/ImageDecoder.h"

#include "raylib.h"

#include <cstddef>

/**
 * @file ImageDecoder.cpp
 * @brief 以 raylib 載入圖片並一律轉為 RGBA8 的解碼實作。
 */

namespace nccu::engine::render {

DecodedImage LoadRgba8Image(const std::string& path) {
    ::Image img = ::LoadImage(path.c_str());
    // 載入失敗（檔案不存在或寬度非正）時回傳空結果；若 raylib 仍配置了
    // 緩衝區則先釋放，避免洩漏。
    if (img.data == nullptr || img.width <= 0) {
        if (img.data) ::UnloadImage(img);
        return DecodedImage{};
    }
    // 統一像素格式為 RGBA8，使上層不必處理各種來源格式。
    ::ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    DecodedImage out;
    out.width  = img.width;
    out.height = img.height;
    const auto pixelCount =
        static_cast<std::size_t>(out.width) *
        static_cast<std::size_t>(out.height) * 4u;
    const auto* src = static_cast<const std::uint8_t*>(img.data);
    out.rgba8.assign(src, src + pixelCount);

    // 像素已複製進 out，raylib 的緩衝區可立即釋放，避免持有兩份。
    ::UnloadImage(img);
    return out;
}

} // namespace nccu::engine::render
