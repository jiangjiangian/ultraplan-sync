#include "ui/overlay/MenuAffordance.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

namespace nccu {

void DrawMenuAffordance(nccu::engine::render::IRenderer& r,
                        const World& world,
                        float screenW,
                        float /*screenH*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // Top-right affordance: a small always-on hint that an in-game menu
    // exists ("M 選單"). Panel-backed so it stays legible on any tile;
    // hidden while the menu itself is open (the menu replaces it).
    if (world.MenuOpen()) return;

    constexpr float kAffSize = 14.0f;
    constexpr float kPad     = 5.0f;
    const std::string aff = "M 選單";
    int glyphs = 0;
    for (unsigned char c : aff)
        if ((c & 0xC0) != 0x80) ++glyphs;
    // "M " is 1 narrow + 選單 2 wide → estimate width generously.
    const float tw = static_cast<float>(glyphs) * kAffSize;
    const float panelW = tw + kPad * 2.0f;
    const float panelH = kAffSize + kPad * 2.0f;
    const float px = screenW - panelW - 6.0f;
    r.DrawRect(Rect{px, 6.0f, panelW, panelH},
               Color{20, 22, 30, 170});
    TextBuilder{aff}.At(Vec2{px + kPad, 6.0f + kPad})
        .Size(static_cast<int>(kAffSize)).Color(Colors::White).Draw();
}

}  // namespace nccu
