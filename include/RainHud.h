#ifndef RAIN_HUD_H_
#define RAIN_HUD_H_
#include <string_view>

namespace nccu {

// Colour-blind redundancy for the rain HUD readout (audit D2, SC 1.4.1).
// Returns a fixed 2-char prefix that mirrors the colour ramp's three
// pressure tiers so deuteran/protan viewers — who cannot distinguish
// white→gold→red reliably — still get the rising-risk signal:
//   rm < 60       → "  "  (calm)
//   60 ≤ rm < 85  → " !"  (warning)
//   rm ≥ 85       → "!!"  (critical)
// Pure: no raylib, no allocation; the returned view points at a static
// literal so it is safe to hold for the lifetime of the program. The
// View prepends this to "rain: NN%" so the text channel never carries
// information by colour alone.
constexpr std::string_view RainTierPrefix(float rm) noexcept {
    if (rm >= 85.0f) return "!!";
    if (rm >= 60.0f) return " !";
    return "  ";
}

} // namespace nccu

#endif // RAIN_HUD_H_
