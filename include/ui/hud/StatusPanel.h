#ifndef UI_HUD_STATUS_PANEL_H_
#define UI_HUD_STATUS_PANEL_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// Top-left HUD status panel (P1 step 7a — extracted from View::RenderHud).
//
// Render-only, screen-space: a translucent black backing rect + up to 7
// rows of text — WASD hint, Tab/M control hint, karma+umbrella,
// 金幣 (money), Inside (when in a building), chapter name, rain
// readout. Width is estimated from the actual UTF-8 codepoint counts so
// the backing hugs the widest row (chapter / 金幣 are CJK ⇒ ×2 wide).
//
// All values are read from the supplied World (and its Player); the
// function never mutates state (MVC §5 — "View = render only"). No
// raylib calls; everything routes through the injected IRenderer, so it
// is deterministic and the headless harness exercises it byte-for-byte.
//
// Anchored at the fixed (10, 10) screen offset that the original code
// used; no caller adjustment needed.
void DrawStatusPanel(nccu::gfx::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_HUD_STATUS_PANEL_H_
