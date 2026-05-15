#include "TransparentUmbrella.h"
#include "gfx/IRenderer.h"

void TransparentUmbrella::Render(nccu::gfx::IRenderer& renderer) const {
    // 3-rect umbrella glyph inside the item's 20x20 footprint: a tapered
    // canopy in umbrellaTint_ plus a dark handle. Template Method — the
    // skeleton lives here; the four subclasses only supply their distinct
    // tint via the constructor, so they read at a glance on the map.
    const float x = position_.x;
    const float y = position_.y;
    renderer.DrawRect(nccu::gfx::Rect{x + 2.0f, y +  4.0f, 16.0f, 3.0f}, umbrellaTint_);
    renderer.DrawRect(nccu::gfx::Rect{x + 0.0f, y +  7.0f, 20.0f, 3.0f}, umbrellaTint_);
    renderer.DrawRect(nccu::gfx::Rect{x + 9.0f, y + 10.0f,  2.0f, 9.0f},
                      nccu::gfx::Colors::DarkGray);
}
