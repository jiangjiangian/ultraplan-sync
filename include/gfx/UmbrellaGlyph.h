#ifndef GFX_UMBRELLA_GLYPH_H_
#define GFX_UMBRELLA_GLYPH_H_
#include "engine/math/Color.h"
#include "gfx/IRenderer.h"
#include "engine/math/Rect.h"

// SINGLE SOURCE OF TRUTH for how every umbrella variant is drawn — a pure
// rect-only vector glyph the owner pinned in the iteration brief. The same
// helper draws the umbrella IN-WORLD (TransparentUmbrella::Render), as a
// ground PICKUP (QuestFlagPickup::Render — the Ch1 苦主 transparent umbrella),
// and on the ENDING SCREEN (EndingView keyed to the ending), so a given
// umbrella looks identical everywhere it appears. No raylib, no GL: every
// pixel goes through the injected IRenderer's DrawRect, exactly like the rest
// of the Item render path (Items must not call DrawText/DrawTexture — the
// architecture rule). resources/ may be empty, so a shape-drawn umbrella
// renders the same with or without assets.
//
// The owner's four umbrellas + the ProfessorTrap variant:
//   TrueBlue        真傘  —  藍色, a wide rounded canopy (the clean / correct).
//   FragileBroken   破傘  —  ONLY the handle + bare bent ribs survive (canopy
//                            gone): the "易碎/壞掉" read the owner asked for.
//   CursedPurple    詛咒傘 — 暗紫, a sagging heavy canopy + black handle.
//   UglyGreen       醜傘  —  螢光綠, a lumpy garish canopy (impossible to mix up).
//   ProfessorTrap   陷阱傘 — warning red, an angular spiked canopy (a trap).
//                            NOT one of the owner's four; given a distinct
//                            danger-red look so it can never be confused with
//                            the true (blue) or ugly (green) umbrella.
namespace nccu::gfx {

enum class UmbrellaLook {
    TrueBlue,       // 真傘 — blue
    FragileBroken,  // 破傘 — only the handle/ribs remain
    CursedPurple,   // 詛咒傘 — dark purple
    UglyGreen,      // 醜傘 — fluorescent green
    ProfessorTrap   // 陷阱傘 — danger red (not in the owner's four)
};

// The canonical signature colour of each look — the value the in-world
// umbrellas, the pickup and the ending card all share. Exposed so callers
// (e.g. a HUD swatch) can match the glyph without re-deriving it.
[[nodiscard]] constexpr Color UmbrellaLookColor(UmbrellaLook look) noexcept {
    switch (look) {
        case UmbrellaLook::TrueBlue:      return Color{ 40, 120, 235, 255};
        case UmbrellaLook::FragileBroken: return Color{170, 170, 175, 255};
        case UmbrellaLook::CursedPurple:  return Color{ 90,  40, 120, 255};
        case UmbrellaLook::UglyGreen:     return Color{120, 255,  40, 255};
        case UmbrellaLook::ProfessorTrap: return Color{235,  70,  40, 255};
    }
    return Color{255, 255, 255, 255};
}

// Draw the umbrella glyph for `look` scaled to fill `bounds`. All geometry is
// expressed as fractions of the box so the same silhouette reads at the 20x20
// world footprint, the 16x16 ground pickup, and the large ending card. Pure
// presentation off the passed enum — no sim/state, MVC clean.
//
// `alpha` (0..255) scales EVERY rect's opacity so a fading surface (the
// ending card fades in via its own alpha) can draw the glyph at the matching
// strength. Defaults to fully opaque for the in-world / pickup callers, which
// never fade — so their behaviour is byte-unchanged.
inline void DrawUmbrellaGlyph(IRenderer& r, UmbrellaLook look, Rect bounds,
                              unsigned char alpha = 255) {
    namespace C = Colors;
    const float x = bounds.x;
    const float y = bounds.y;
    const float w = bounds.width;
    const float h = bounds.height;
    // Pre-scale a colour's alpha by the glyph alpha (255 ⇒ unchanged).
    auto fade = [alpha](Color col) {
        return alpha == 255 ? col
            : col.WithAlpha(static_cast<unsigned char>(
                  static_cast<int>(col.a) * static_cast<int>(alpha) / 255));
    };
    // rc(fx,fy,fw,fh): a rect placed/sized in box fractions, alpha-scaled.
    auto rc = [&](float fx, float fy, float fw, float fh, Color col) {
        r.DrawRect(Rect{x + fx * w, y + fy * h, fw * w, fh * h}, fade(col));
    };
    const Color t = UmbrellaLookColor(look);

    switch (look) {
        case UmbrellaLook::TrueBlue: {
            // A wide, full, rounded 3-tier canopy — the "complete / correct"
            // read — over a light handle so the clear umbrella looks intact.
            rc(0.30f, 0.10f, 0.40f, 0.10f, t);
            rc(0.15f, 0.20f, 0.70f, 0.15f, t);
            rc(0.00f, 0.35f, 1.00f, 0.15f, t);
            rc(0.45f, 0.50f, 0.10f, 0.40f, C::DarkGray);   // shaft
            rc(0.35f, 0.88f, 0.30f, 0.10f, C::DarkGray);   // hook foot
            break;
        }
        case UmbrellaLook::FragileBroken: {
            // ONLY the handle and a few bare, bent ribs remain — the canopy is
            // GONE. Reads unmistakably as "壞掉/剩手柄" (the owner's spec). A
            // top rib-hub, three splayed broken ribs, then the bent shaft+hook.
            rc(0.43f, 0.16f, 0.14f, 0.10f, t);             // rib hub (top)
            rc(0.14f, 0.20f, 0.26f, 0.05f, t);             // left rib (snapped, drooping)
            rc(0.10f, 0.25f, 0.06f, 0.10f, t);             // left rib tip hanging down
            rc(0.60f, 0.20f, 0.24f, 0.05f, t);             // right rib (snapped)
            rc(0.84f, 0.25f, 0.06f, 0.08f, t);             // right rib tip
            rc(0.46f, 0.26f, 0.08f, 0.52f, C::DarkGray);   // shaft
            rc(0.40f, 0.78f, 0.22f, 0.08f, C::DarkGray);   // bent hook foot
            break;
        }
        case UmbrellaLook::CursedPurple: {
            // A sagging, heavy canopy whose ribs hang DOWN at the sides, over a
            // pure-black handle — an oppressive "this is wrong" read.
            rc(0.35f, 0.14f, 0.30f, 0.10f, t);
            rc(0.15f, 0.24f, 0.70f, 0.14f, t);
            rc(0.05f, 0.38f, 0.90f, 0.10f, t);
            rc(0.05f, 0.48f, 0.15f, 0.20f, t);             // left rib drooping down
            rc(0.80f, 0.48f, 0.15f, 0.20f, t);             // right rib drooping down
            rc(0.45f, 0.48f, 0.10f, 0.46f, C::Black);      // black shaft
            break;
        }
        case UmbrellaLook::UglyGreen: {
            // A lumpy, garish, asymmetric canopy in screaming fluorescent
            // green — "醜得全校最好認" (ugly enough no one grabs it by mistake).
            rc(0.25f, 0.12f, 0.55f, 0.12f, t);             // off-centre top blob
            rc(0.10f, 0.24f, 0.85f, 0.16f, t);             // bulging mid (skewed right)
            rc(0.05f, 0.40f, 0.80f, 0.12f, t);             // lopsided hem
            rc(0.85f, 0.36f, 0.12f, 0.10f, t);             // a stray ugly bump
            rc(0.42f, 0.52f, 0.10f, 0.40f, C::DarkGray);   // shaft
            rc(0.32f, 0.90f, 0.30f, 0.08f, C::DarkGray);   // hook foot
            break;
        }
        case UmbrellaLook::ProfessorTrap: {
            // An angular STEPPED pyramid canopy with a sharp tip + jagged hem
            // barbs: an alarming, weaponised "this is a trap" silhouette.
            rc(0.43f, 0.04f, 0.10f, 0.14f, t);             // spike tip
            rc(0.33f, 0.18f, 0.34f, 0.10f, t);
            rc(0.18f, 0.28f, 0.64f, 0.10f, t);
            rc(0.04f, 0.38f, 0.92f, 0.10f, t);
            rc(0.08f, 0.48f, 0.10f, 0.10f, t);             // hem barbs
            rc(0.45f, 0.48f, 0.10f, 0.10f, t);
            rc(0.82f, 0.48f, 0.10f, 0.10f, t);
            rc(0.45f, 0.58f, 0.10f, 0.36f, C::DarkGray);   // shaft
            break;
        }
    }
}

} // namespace nccu::gfx

#endif // GFX_UMBRELLA_GLYPH_H_
