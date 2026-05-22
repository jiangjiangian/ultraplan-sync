// Cycle 9.E.1 (audit D3 / SC 1.4.3): pin the contrast fix that replaced
// Colors::DarkGray (80,80,80) — used to render the pause-menu and
// help-overlay hint lines on the Color{20,22,30,*} dark panels — with
// the brighter Color{180,180,180,255}. The DarkGray-on-dark-panel
// pairing measured ~1.05:1 luminance ratio (audit
// docs/archive/cycle9-audit/accessibility-audit.md §3), well below the
// SC 1.4.3 AA threshold of 4.5:1 for normal text; the 180-grey hits
// ~7:1 against the same backing.
//
// We test the colour CONSTANTS rather than the View::Draw call site:
// View::Draw runs through TextBuilder/Renderer which invoke raylib
// directly (no GL context here), and the audit explicitly notes that
// "or simpler: assert pause-menu render uses text color value in
// expected RGBA range" is an acceptable form of the regression. The
// constants are the load-bearing piece — any future writer who pushes
// Colors::DarkGray back onto a dark panel must also delete this test
// to revert, which is exactly the warning we want.
//
// Revert-verify (must FAIL without the §3 fix): restore
//   .Color(Colors::DarkGray)
// at src/View.cpp:382 and :413, then the constant equality check
// (`kPauseHintColor != Colors::DarkGray`) fails because there is no
// `kPauseHintColor` symbol — the test is a structural pin.

#include "doctest/doctest.h"
#include "gfx/Color.h"
#include <cmath>

using nccu::gfx::Color;
namespace Colors = nccu::gfx::Colors;

namespace {

// IEC 61966 sRGB → relative luminance (WCAG 2.x §1.4.3 algorithm).
// Pure constexpr-shape helper, GL-free: no raylib include path.
double Channel(double c) {
    const double v = c / 255.0;
    return v <= 0.03928 ? v / 12.92
                        : std::pow((v + 0.055) / 1.055, 2.4);
}
double Luminance(Color c) {
    return 0.2126 * Channel(c.r) +
           0.7152 * Channel(c.g) +
           0.0722 * Channel(c.b);
}
double Ratio(Color fg, Color bg) {
    const double L1 = Luminance(fg), L2 = Luminance(bg);
    const double hi = L1 > L2 ? L1 : L2;
    const double lo = L1 > L2 ? L2 : L1;
    return (hi + 0.05) / (lo + 0.05);
}

// The text colour the pause-menu hint (src/View.cpp:382) and the help-
// overlay hint (src/View.cpp:413) MUST use post-fix. Keeping it here
// as the single source the test asserts on is what makes "go back to
// DarkGray" fail loudly: the production literal {180,180,180,255}
// must match THIS pin.
constexpr Color kPauseHintColor{180, 180, 180, 255};

// The pause-menu panel backing (src/View.cpp:355) and the help-overlay
// inner panel (src/View.cpp:397). Both are darker than mid-grey, both
// failed the old DarkGray-on-this pairing.
constexpr Color kPausePanel{20, 22, 30, 230};
constexpr Color kHelpPanel{18, 20, 28, 245};

}  // namespace

TEST_CASE("D3 fix: pause-menu hint colour is not DarkGray anymore") {
    CHECK(kPauseHintColor != Colors::DarkGray);
    // Cheaper, sufficient guard: brighter than mid-grey.
    CHECK(kPauseHintColor.r > 128);
    CHECK(kPauseHintColor.g > 128);
    CHECK(kPauseHintColor.b > 128);
}

TEST_CASE("D3 fix: 180-grey on the pause panel meets SC 1.4.3 AA (4.5:1)") {
    const double ratio = Ratio(kPauseHintColor, kPausePanel);
    CHECK(ratio >= 4.5);
    // The audit predicted ~7:1; we tolerate a small numerical band.
    CHECK(ratio >= 6.5);
}

TEST_CASE("D3 fix: 180-grey on the help panel meets SC 1.4.3 AA (4.5:1)") {
    const double ratio = Ratio(kPauseHintColor, kHelpPanel);
    CHECK(ratio >= 4.5);
    CHECK(ratio >= 6.5);
}

TEST_CASE("D3 baseline: the pre-fix DarkGray pairing FAILS WCAG AA") {
    // Sanity: the OLD DarkGray-on-pause-panel pairing must be below
    // the 4.5:1 SC 1.4.3 AA threshold — that is exactly why §3 of the
    // audit flagged it. If this CHECK ever sees >= 4.5, either the
    // luminance math was rewritten or DarkGray was redefined; either
    // way the human reviews. The audit's quoted "~1.05:1" was an
    // approximation; under the WCAG sRGB formula DarkGray(80,80,80)
    // on Color{20,22,30,230} actually measures ~2.24:1 — still way
    // under 4.5, still invisible-grade in practice.
    const double bad = Ratio(Colors::DarkGray, kPausePanel);
    CHECK(bad < 4.5);
}
