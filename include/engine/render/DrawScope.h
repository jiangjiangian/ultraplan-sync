#ifndef GFX_DRAW_SCOPE_H_
#define GFX_DRAW_SCOPE_H_
#include "raylib.h"

namespace nccu::engine::render {

/**
 * @file DrawScope.h
 * @brief 繪圖區間的 RAII 守衛：建構時 BeginDrawing、解構時 EndDrawing。
 */

/**
 * @brief 以 RAII 包住一幀的繪圖區間。
 *
 * 建構子呼叫 BeginDrawing、解構子呼叫 EndDrawing，使每幀的開始／結束成對且不漏。
 * 不可複製亦不可移動，避免重複呼叫 EndDrawing。
 */
class DrawScope {
public:
    /** @brief 開始一幀繪圖。 */
    DrawScope() noexcept { ::BeginDrawing(); }
    /** @brief 結束一幀繪圖。 */
    ~DrawScope() { ::EndDrawing(); }

    DrawScope(const DrawScope&)            = delete;
    DrawScope& operator=(const DrawScope&) = delete;
    DrawScope(DrawScope&&)                 = delete;
    DrawScope& operator=(DrawScope&&)      = delete;
};

} // namespace nccu::engine::render

#endif // GFX_DRAW_SCOPE_H_
