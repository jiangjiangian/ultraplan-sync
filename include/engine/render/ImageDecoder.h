#ifndef ENGINE_RENDER_IMAGE_DECODER_H_
#define ENGINE_RENDER_IMAGE_DECODER_H_
#include <cstdint>
#include <string>
#include <vector>

namespace nccu::engine::render {

/**
 * @file ImageDecoder.h
 * @brief 影像解碼邊界：把 raylib.h 限制在 .cpp 內，對外只暴露純資料的 RGBA8 像素緩衝。
 */

/**
 * @brief 已解碼的 RGBA8 影像（純資料）。
 *
 * 由下方 LoadRgba8Image 回傳。遊戲領域的呼叫者（如 game/world/MaskLoader.h 等）
 * 可直接消費像素緩衝，而完全不必 #include raylib.h。
 */
struct DecodedImage {
    int                       width  = 0;   ///< 寬（像素）
    int                       height = 0;   ///< 高（像素）
    std::vector<std::uint8_t> rgba8;   ///< 像素資料；大小 = width * height * 4

    /** @brief 是否為空影像（無有效尺寸或無像素）。@return 空回傳 true。 */
    [[nodiscard]] bool Empty() const noexcept {
        return width <= 0 || height <= 0 || rgba8.empty();
    }
};

/**
 * @brief 將檔案載入為 RGBA8 像素緩衝。
 * @param[in] path 影像檔路徑。
 * @return 解碼後的影像；檔案缺失／解碼失敗時回傳空的 DecodedImage（Empty() == true），絕不丟例外。
 *
 * 呼叫者依情況自行警告或退路處理。實作把每個 ::LoadImage／::ImageFormat／
 * ::UnloadImage 呼叫都藏在 .cpp，使此標頭不引入任何 raylib 符號。
 */
[[nodiscard]] DecodedImage LoadRgba8Image(const std::string& path);

} // namespace nccu::engine::render

#endif // ENGINE_RENDER_IMAGE_DECODER_H_
