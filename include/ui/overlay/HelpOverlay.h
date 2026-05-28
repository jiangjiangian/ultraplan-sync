#ifndef UI_OVERLAY_HELP_OVERLAY_H_
#define UI_OVERLAY_HELP_OVERLAY_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// In-game 說明 (how-to-play) overlay (P1 step 7d — extracted from
// View::RenderOverlays).
//
// Drawn ABOVE the paused menu (which sits behind it). A pure function
// of World::MenuOpen + HelpOpen + HelpPage; the same shared GameHelp
// text the title screen uses, so the two never drift. A full-screen
// scrim + one paged help body + a "M / E 返回選單" chip. Safe to call
// every frame: early-returns unless the menu AND help are both open.
//
// Render-only (MVC §5): reads World const, never mutates. Routes draws
// through the injected IRenderer + the shared `nccu::ui::DrawHelpPage`
// helper used by the title screen, so the overlay can never drift from
// the splash-screen 說明 view.
void DrawHelpOverlay(nccu::gfx::IRenderer& r,
                     const World& world,
                     float screenW,
                     float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_HELP_OVERLAY_H_
