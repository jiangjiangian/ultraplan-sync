#ifndef UI_OVERLAY_MENU_AFFORDANCE_H_
#define UI_OVERLAY_MENU_AFFORDANCE_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// Top-right "M 選單" affordance hint (P1 step 7c — extracted from
// View::RenderOverlays). A small panel-backed "M 選單" chip that
// always tells the player how to summon the pause menu.
//
// Reactive: a pure function of World::MenuOpen — the chip is hidden
// while the menu itself is open (the menu replaces it). Safe to call
// every frame; early-returns when MenuOpen is true. No retained UI
// state.
//
// Render-only (MVC §5): reads World const, never mutates. Routes all
// draws through the injected IRenderer + the gfx TextBuilder, so it
// is deterministic and headless-spy-testable.
//
// `screenW` / `screenH` are the framebuffer dimensions; the chip
// anchors to the top-right corner (6 px inset).
void DrawMenuAffordance(nccu::gfx::IRenderer& r,
                        const World& world,
                        float screenW,
                        float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_MENU_AFFORDANCE_H_
