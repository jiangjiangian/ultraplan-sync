#include "ui/overlay/HelpOverlay.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "ui/GameHelp.h"
#include "ui/HelpPageView.h"

#include <algorithm>

namespace nccu {

void DrawHelpOverlay(nccu::engine::render::IRenderer& r,
                     const World& world,
                     float screenW,
                     float screenH) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    // REQUIREMENT #9 + U2-T4: the in-game 說明 (how-to-play) overlay —
    // drawn ABOVE the menu (which is still up behind it). Pure function of
    // World::HelpOpen() + World::HelpPage(); the same shared GameHelp text
    // the title screen uses, so the two never drift. A near-full-screen
    // panel showing one PAGE of the (now-paged) help; ←/→ flip the page,
    // M/E/Enter (all handled in GameController) dismiss it back to the menu
    // (ESC quits the program).
    if (!(world.MenuOpen() && world.HelpOpen())) return;

    const float W = screenW;
    const float H = screenH;
    // Full-screen scrim ON TOP of the paused menu (overlay-only — the
    // title screen Clears instead, so this stays at the call site).
    r.DrawRect(Rect{0.0f, 0.0f, W, H}, Color{0, 0, 0, 205});
    // Shared 遊戲說明 page body (review MINOR de-dup): same panel/title/
    // paged body/indicator/返回 chip the title screen draws. The overlay
    // values: 245α panel, light indicator, "M / E 返回選單" chip at -58.
    nccu::ui::DrawHelpPage(
        [&r](Rect rect, Color c) { r.DrawRect(rect, c); },
        nccu::ui::HelpPageStyle{
            W, H,
            std::max(0, std::min(world.HelpPage(),
                                 nccu::kGameHelpPageCount - 1)),
            Color{18, 20, 28, 245},
            Color{200, 200, 210, 255},
            "M / E 返回選單", -58.0f});
}

}  // namespace nccu
