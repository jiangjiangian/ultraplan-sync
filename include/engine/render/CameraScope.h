#ifndef GFX_CAMERA_SCOPE_H_
#define GFX_CAMERA_SCOPE_H_
#include "raylib.h"
#include "engine/render/Camera2D.h"

namespace nccu::engine::render {

/**
 * @file CameraScope.h
 * @brief 攝影機模式的 RAII 守衛：建構時 BeginMode2D、解構時 EndMode2D。
 */

/**
 * @brief 以 RAII 包住 raylib 的 2D 攝影機模式。
 *
 * 建構子呼叫 BeginMode2D、解構子呼叫 EndMode2D。必須建立於 DrawScope「之內」並在
 * DrawScope 結束「之前」解構——在 EndDrawing 之後才呼叫 EndMode2D 在 raylib 中為
 * 未定義行為；以內層 `{}` 區塊的堆疊順序自然保證此點。
 *
 * 不可複製亦不可移動：被移走的 CameraScope 否則會在解構時重複呼叫 EndMode2D。
 */
class CameraScope {
public:
    /** @brief 進入 2D 攝影機模式。@param[in] cam 攝影機參數。 */
    explicit CameraScope(const Camera2D& cam) noexcept {
        ::BeginMode2D(::Camera2D{
            ::Vector2{cam.offset.x, cam.offset.y},
            ::Vector2{cam.target.x, cam.target.y},
            cam.rotation,
            cam.zoom
        });
    }
    ~CameraScope() { ::EndMode2D(); }

    CameraScope(const CameraScope&)            = delete;
    CameraScope& operator=(const CameraScope&) = delete;
    CameraScope(CameraScope&&)                 = delete;
    CameraScope& operator=(CameraScope&&)      = delete;
};

} // namespace nccu::engine::render

#endif // GFX_CAMERA_SCOPE_H_
