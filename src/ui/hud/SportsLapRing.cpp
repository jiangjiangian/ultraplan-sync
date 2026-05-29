#include "ui/hud/SportsLapRing.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <cmath>

namespace nccu {

void DrawSportsLapRing(nccu::engine::render::IRenderer& r,
                       const World& world,
                       float screenW,
                       float /*screenH*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // 操場 校慶 lap progress ring (HUD, screen space) — fills clockwise as
    // the lap completes; the screen companion to the ground track.
    if (!world.SportsLapActive()) return;

    const float prog = world.SportsLapProgress();
    constexpr int kDots = 16;
    constexpr float kTwoPi = 6.2831853f;
    const float cx = screenW - 60.0f, cy = 120.0f, ringR = 24.0f;
    for (int i = 0; i < kDots; ++i) {
        const float frac = static_cast<float>(i) /
                           static_cast<float>(kDots);
        const float a = -kTwoPi * 0.25f + frac * kTwoPi;  // top, clockwise
        const float x = cx + ringR * std::cos(a);
        const float y = cy + ringR * std::sin(a);
        r.DrawRect(Rect{x - 3.0f, y - 3.0f, 6.0f, 6.0f},
                   frac < prog ? Color{255, 230, 90, 255}
                               : Color{255, 255, 255, 70});
    }
}

}  // namespace nccu
