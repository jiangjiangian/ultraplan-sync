#ifndef UI_OVERLAY_PAUSE_MENU_H_
#define UI_OVERLAY_PAUSE_MENU_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// In-game pause menu overlay (P1 step 7b — extracted from
// View::RenderOverlays).
//
// Full-screen dim + centred panel + 6 rows (繼續 / 說明 / 減少動畫 /
// 擴大目標 / 重新開始 / 離開) + a keyboard-help band along the bottom.
// Reactive: a pure function of World::MenuOpen + MenuCursor +
// ReducedMotion + LargeTargets (the two toggle rows). No retained UI
// state in the View — the function is a no-op while MenuOpen is false,
// so it is safe to call every frame.
//
// Render-only (MVC §5): reads World const, never mutates. All draws
// route through the injected IRenderer + the gfx TextBuilder, so it is
// deterministic and headless-spy-testable.
//
// `screenW` / `screenH` are the framebuffer dimensions; the dim and
// the panel centre on them.
void DrawPauseMenu(nccu::gfx::IRenderer& r,
                   const World& world,
                   float screenW,
                   float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_PAUSE_MENU_H_
