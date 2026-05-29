// Cycle 9.E.1 (audit D2 / SC 1.4.1): pin the colour-blind redundancy
// added to the rain HUD readout in src/View.cpp. The text channel
// previously carried tier information only through the white→gold→red
// colour ramp; deuteran/protan viewers see gold and red as nearly
// identical olive/brown. A 2-character glyph prefix is now prepended
// to the buffer so the tier is legible without relying on hue:
//   rm <  60 → "  "  (calm)
//   60 ≤ rm < 85 → " !"  (warning)
//   rm >= 85 → "!!"  (critical)
// The colour ramp itself is preserved as amplification.
//
// We test the pure helper RainTierPrefix() that View.cpp uses to build
// the buffer. View::Draw itself goes through raylib (no GL context),
// but the audit's exact recommendation was a prefix derived from rm
// in src/View.cpp:243 — so the helper IS the load-bearing logic.
//
// Revert-verify (must FAIL without the M1 fix): restore the old
//   std::snprintf(rbuf, sizeof(rbuf), "rain: %d%%", ...)
// at src/View.cpp:243 and drop include/RainHud.h. Either the prefix
// check fails (no helper), or any future caller printing without
// RainTierPrefix never composes the expected leading marker.

#include "doctest/doctest.h"
#include "ui/RainHud.h"
#include <cstdio>
#include <string>
#include <string_view>

using nccu::RainTierPrefix;

TEST_CASE("D2 fix: RainTierPrefix returns calm/warn/crit by threshold") {
    // Below 60: calm — two spaces. Spaces preserve the column so the
    // "rain: NN%" digits still align with the other HUD lines.
    CHECK(RainTierPrefix(0.0f)   == "  ");
    CHECK(RainTierPrefix(30.0f)  == "  ");
    CHECK(RainTierPrefix(59.99f) == "  ");

    // [60, 85): warning — single '!'.
    CHECK(RainTierPrefix(60.0f)  == " !");
    CHECK(RainTierPrefix(72.5f)  == " !");
    CHECK(RainTierPrefix(84.99f) == " !");

    // >= 85: critical — double '!!'.
    CHECK(RainTierPrefix(85.0f)  == "!!");
    CHECK(RainTierPrefix(90.0f)  == "!!");
    CHECK(RainTierPrefix(100.0f) == "!!");
}

TEST_CASE("D2 fix: the prefix is exactly 2 visible cells in every tier") {
    // Width invariant: the rain HUD line is composed against fixed
    // gutter so the prefix MUST be a fixed 2-cell glyph regardless
    // of tier (no shrink to "!", no stretch to "!!!"). Keeps the
    // column alignment with karma/umbrella/金幣 above and below it.
    CHECK(RainTierPrefix(0.0f).size()   == 2u);
    CHECK(RainTierPrefix(70.0f).size()  == 2u);
    CHECK(RainTierPrefix(95.0f).size()  == 2u);
}

TEST_CASE("D2 fix: a HUD line at mid tier carries the warn glyph") {
    // End-to-end shape: replicate the View::Draw snprintf call that
    // builds rbuf, then check the resulting text. Confirms the helper
    // composes correctly into the format string the View uses.
    constexpr float midTier = 70.0f;
    const std::string_view tag = RainTierPrefix(midTier);
    char rbuf[40] = {0};
    std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
        static_cast<int>(tag.size()), tag.data(),
        static_cast<int>(midTier + 0.5f));
    const std::string s(rbuf);
    CHECK(s.find(" !") != std::string::npos);
    CHECK(s.find("rain: 70%") != std::string::npos);
}

TEST_CASE("D2 fix: a HUD line at critical tier carries the !! glyph") {
    constexpr float critTier = 90.0f;
    const std::string_view tag = RainTierPrefix(critTier);
    char rbuf[40] = {0};
    std::snprintf(rbuf, sizeof(rbuf), "%.*s rain: %d%%",
        static_cast<int>(tag.size()), tag.data(),
        static_cast<int>(critTier + 0.5f));
    const std::string s(rbuf);
    CHECK(s.find("!!") != std::string::npos);
    CHECK(s.find("rain: 90%") != std::string::npos);
}
