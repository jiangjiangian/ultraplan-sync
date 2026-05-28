#include "entities/DlcSign.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

#include <string>

DlcSign::DlcSign(nccu::gfx::Vec2 position)
    // Direct base is WithRoles<DlcSign, GameObject>; its `using Base::Base`
    // inherits GameObject's (position, hitBox) ctor. A 28x28 hitbox — a touch
    // bigger than the 16x16 pickups so the bold "?" reads as a standee and is
    // comfortably reachable by the E-probe (inflated to ±8 px in
    // GameController) from a flush-blocked approach.
    : WithRoles(position,
           nccu::gfx::Rect{position.x, position.y, 28.0f, 28.0f}),
      // B2 / UI-B-3: literal '\n' is honoured by MessageView::WrapCjk (it
      // breaks the toast at the newline) AND each wrapped row is now centred
      // in the toast box, so the teaser reads as two tidy centred lines:
      //   DLC開發中
      //   敬請期待
      // (the 【…】 brackets / … ellipsis were dropped per UI-B-3 so the two
      // lines balance cleanly when centred). 敬 is baked into the font atlas
      // (gfx::Font.h UiLiteralChars), enforced by test_font_ui_literal_scan.
      message_("DLC開發中\n敬請期待") {}

void DlcSign::Render(nccu::gfx::IRenderer& renderer) const {
    using nccu::gfx::Rect;
    namespace C = nccu::gfx::Colors;

    // A bold "?" drawn rect-only (Item/scenery must not call DrawText/
    // DrawTexture — the architecture rule; no raylib), in the same
    // box-fraction style as UmbrellaGlyph so it scales with the hitbox.
    // First a dark plate so the bright "?" reads against any ground, then
    // the glyph strokes in gold (an "interact me" cue colour).
    const float x = hitBox_.x;
    const float y = hitBox_.y;
    const float w = hitBox_.width;
    const float h = hitBox_.height;
    auto rc = [&](float fx, float fy, float fw, float fh, nccu::gfx::Color col) {
        renderer.DrawRect(Rect{x + fx * w, y + fy * h, fw * w, fh * h}, col);
    };

    // Backing plate (full hitbox) — a dark signboard.
    rc(0.00f, 0.00f, 1.00f, 1.00f, nccu::gfx::Color{30, 32, 40, 220});

    // "?" strokes, gold. Top hook + the curve down to the centre, a gap,
    // then the dot — a chunky, unmistakable question mark.
    const nccu::gfx::Color q = C::Gold;
    rc(0.28f, 0.14f, 0.44f, 0.12f, q);   // top bar of the hook
    rc(0.62f, 0.14f, 0.16f, 0.30f, q);   // right descent of the hook
    rc(0.40f, 0.40f, 0.30f, 0.12f, q);   // sweep in toward the stem
    rc(0.42f, 0.50f, 0.16f, 0.20f, q);   // vertical stem down to the gap
    rc(0.42f, 0.80f, 0.16f, 0.12f, q);   // the dot
}

void DlcSign::Interact(Player* /*initiator*/) {
    // Re-readable: publish the teaser, but do NOT deactivate — the sign
    // stays in the world so the player can read it again. No gameplay
    // effect (no flag / karma / money), so the initiator is unused.
    nccu::events::Sink().Publish(
        Event{EventType::ShowMessage, message_});
}
