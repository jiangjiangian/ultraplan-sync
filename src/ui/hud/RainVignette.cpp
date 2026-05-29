#include "ui/hud/RainVignette.h"

#include "game/entities/Player.h"
#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <algorithm>

namespace nccu {

void DrawRainVignette(nccu::engine::render::IRenderer& r,
                      const World& world,
                      float screenW,
                      float screenH) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // Rain "pressure" vignette — PURE render derived only from
    // GetRainMeter() (no sim/state/input touched: MVC §5). The rain is
    // non-lethal this cycle; the feedback is purely visual. Screen-edge
    // darkening in two tiers (≥60 subtle, ≥85 stronger) drawn as four
    // border bands (cheap, no full-screen texture/alloc, deterministic).
    const Player* p = world.GetPlayer();
    if (!p) return;

    const float rm = p->GetRainMeter();
    unsigned char va = 0;
    if (rm >= 85.0f)      va = 90;
    else if (rm >= 60.0f) va = 45;
    if (va == 0) return;

    const Color v{0, 0, 0, va};
    const float W = screenW;
    const float H = screenH;
    const float b = std::min(W, H) * 0.12f;  // band thickness
    r.DrawRect(Rect{0.0f, 0.0f, W, b}, v);          // top
    r.DrawRect(Rect{0.0f, H - b, W, b}, v);         // bottom
    r.DrawRect(Rect{0.0f, 0.0f, b, H}, v);          // left
    r.DrawRect(Rect{W - b, 0.0f, b, H}, v);         // right
}

}  // namespace nccu
