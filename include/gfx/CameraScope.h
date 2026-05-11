#pragma once
#include "raylib.h"
#include "gfx/Camera2D.h"

namespace nccu::gfx {

// RAII: BeginMode2D in the ctor, EndMode2D in the dtor. Must be created
// INSIDE a DrawScope and destroyed BEFORE the DrawScope ends — calling
// EndMode2D after EndDrawing is undefined behaviour in raylib. Stack
// ordering with an inner `{}` block enforces this naturally.
//
// Non-copyable AND non-movable: a moved-from CameraScope would otherwise
// double-EndMode2D in its destructor.
class CameraScope {
public:
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

} // namespace nccu::gfx
