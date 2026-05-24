#include "doctest/doctest.h"
#include "ui/EndingView.h"
#include "state/SemesterState.h"
#include "gfx/IRenderer.h"
#include "gfx/UmbrellaGlyph.h"
#include <string>
#include <vector>

using nccu::SemesterState;
using nccu::EndingSummary;

namespace {

// Spy IRenderer — same shape as the proven spy in
// test_dialog_box_render.cpp: counts rects (and captures their colours, so
// the T3 ending-umbrella swatch is assertable) and captures text strings, so
// the ending card's draw calls can be asserted without a GL context.
struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    int sprites = 0;
    std::vector<nccu::gfx::Color> rectColors;
    std::vector<std::string> texts;

    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color c) override {
        ++rects;
        rectColors.push_back(c);
    }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override { ++sprites; }
    void DrawText(std::string_view t, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override { texts.emplace_back(t); }
};

// True if any captured text contains `needle` as a substring.
bool Has(const Spy& s, std::string_view needle) {
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) return true;
    return false;
}

int CountWith(const Spy& s, std::string_view needle) {
    int n = 0;
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) ++n;
    return n;
}

// True if any captured rect uses RGB == want (ignoring alpha, which the card
// fade scales). The umbrella glyph's signature colour is its top canopy slab.
bool HasRectRGB(const Spy& s, nccu::gfx::Color want) {
    for (const nccu::gfx::Color& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

} // namespace

TEST_CASE("IsEndingState is true only for Ending_* states") {
    CHECK(nccu::IsEndingState(SemesterState::Ending_C));
    CHECK_FALSE(nccu::IsEndingState(SemesterState::Chapter1_AddDrop));
}

TEST_CASE("DrawEndingCard issues a backdrop + title + caption + stats") {
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.boughtUgly = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(r.rects >= 2);              // backdrop + 結算 panel
    CHECK(r.texts.size() >= 4);       // title + 字卡 + reason + stats rows
}

// Item 1: the card must render the karma NUMBER and the correct in-fiction
// reason / deciding conditions for each ending — driven by the DTO.

TEST_CASE("Ending A card shows karma + all three A deciding conditions") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 92;
    g.hasTrueUmbrella = true;
    g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "92"));              // the actual final karma number
    CHECK(Has(r, "業力"));            // karma readout label
    CHECK(Has(r, "業力 > 80"));       // A condition 1
    CHECK(Has(r, "還回真傘"));         // A condition 2
    CHECK(Has(r, "體諒助教"));         // A condition 3
    CHECK(Has(r, "完美結局"));         // path label
    // A reason line (transcribed from ending_a.md 字卡).
    CHECK(Has(r, "辛苦了"));
}

TEST_CASE("Ending B cold-finale shows ONLY the conditions that fired") {
    // coldFinale = finaleChoiceMade && !consoledTA; karma not <0; not cursed.
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = 35;
    g.finaleChoiceMade = true;
    g.consoledTA = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "35"));
    CHECK(Has(r, "最後質問助教"));      // the fired disjunct
    CHECK_FALSE(Has(r, "拿了詛咒傘"));  // NOT fired -> not shown
    CHECK_FALSE(Has(r, "業力低於零"));  // karma 35 >= 0 -> not shown
    CHECK(Has(r, "墮落結局"));
}

TEST_CASE("Ending B cursed+negative-karma shows both fired conditions") {
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = -7;
    g.tookCursed = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "-7"));
    CHECK(Has(r, "拿了詛咒傘"));
    CHECK(Has(r, "業力低於零"));
}

TEST_CASE("Ending C bought-ugly shows the buy condition, not the default") {
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 50;
    g.boughtUgly = true;
    g.finaleChoiceMade = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "50"));
    CHECK(Has(r, "買了醜傘"));
    CHECK_FALSE(Has(r, "平穩收尾"));   // the buy fired, not the default
    CHECK(Has(r, "務實結局"));
}

TEST_CASE("Ending C calm-default shows the 平穩收尾 condition") {
    // finaleChoiceMade && !boughtUgly -> the EndingGate C default branch.
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 60;
    g.finaleChoiceMade = true;
    g.boughtUgly = false;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    CHECK(Has(r, "平穩收尾"));
    CHECK_FALSE(Has(r, "買了醜傘"));
}

TEST_CASE("Ending card draws exactly one selected '結算' header (anchor)") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 81;
    g.hasTrueUmbrella = true;
    g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    CHECK(CountWith(r, "結算") == 1);
}

// T3: the card draws the FINAL umbrella keyed off the ending, so it can never
// mismatch the verdict (the fix for "體諒卻顯示醜傘"). A → 真傘藍, B → 詛咒傘
// 暗紫, C → 醜傘綠 — the shared glyph's signature colour proves which drew.
TEST_CASE("T3: Ending A draws the 真傘 (blue) umbrella swatch") {
    EndingSummary g;
    g.state = SemesterState::Ending_A;
    g.karma = 90; g.hasTrueUmbrella = true; g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 A", 1.0f, 800.0f, 450.0f);
    using nccu::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
}

TEST_CASE("T3: Ending B draws the 詛咒傘 (dark purple) umbrella swatch") {
    EndingSummary g;
    g.state = SemesterState::Ending_B;
    g.karma = -5; g.tookCursed = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 B", 1.0f, 800.0f, 450.0f);
    using nccu::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

TEST_CASE("T3: Ending C draws the 醜傘 (green) umbrella swatch — even when 體諒") {
    // The owner's "體諒卻顯示醜傘" mismatch: derive the swatch from the ENDING,
    // not raw flags. A C ending where the player ALSO consoled the TA still
    // shows the green ugly umbrella, because the run resolved to C.
    EndingSummary g;
    g.state = SemesterState::Ending_C;
    g.karma = 60; g.boughtUgly = true; g.consoledTA = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 C", 1.0f, 800.0f, 450.0f);
    using nccu::gfx::UmbrellaLook;
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

// G1 — the Ending D card DATA contract the UI phase consumes. IsEndingState
// recognises D; the card renders the 破傘 (FragileBroken) swatch + the D
// caption / reason / 風雨同行 path label / the two D deciding conditions
// (體諒 + karma≤80). Pins exactly what the later UI phase must map D→破傘 to.
TEST_CASE("G1: IsEndingState recognises Ending_D") {
    CHECK(nccu::IsEndingState(SemesterState::Ending_D));
}

TEST_CASE("G1: Ending D card draws the 破傘 (FragileBroken) swatch + D copy") {
    EndingSummary g;
    g.state = SemesterState::Ending_D;
    g.karma = 65;                       // 體諒 with karma in [0,80]
    g.consoledTA = true;
    g.finaleChoiceMade = true;
    Spy r;
    nccu::DrawEndingCard(r, g, "結局 D", 1.0f, 800.0f, 450.0f);
    using nccu::gfx::UmbrellaLook;
    // The破傘 glyph the UI phase maps D to — NOT the true/cursed/ugly looks.
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    // D-specific card copy.
    CHECK(Has(r, "65"));                 // final karma number
    CHECK(Has(r, "風雨同行"));            // path label
    CHECK(Has(r, "體諒助教"));            // D condition 1
    CHECK(Has(r, "業力 ≤ 80"));          // D condition 2 (the 差一點點 clause)
    CHECK(Has(r, "傘破了"));              // a D 字卡 / reason fragment
}
