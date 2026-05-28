#include "doctest/doctest.h"
#include "ui/ChapterCard.h"
#include "state/SemesterState.h"
#include "dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "gfx/UmbrellaGlyph.h"
#include <string>
#include <vector>

using nccu::ChapterCardKind;
using nccu::ChapterCardState;
using nccu::ChapterCardForTransition;
using nccu::ChapterCardHeadline;
using nccu::ChapterCardSubtitle;
using S = nccu::SemesterState;

namespace {

// Spy IRenderer — same shape as the ending-card spy: captures text + rect
// colours so the big card's draw calls are assertable without a GL context.
// UI-B-3 also records each text's draw X + font size, so the within-panel
// wrap (no card row spills the box) is assertable.
struct Spy final : nccu::gfx::IRenderer {
    int rects = 0;
    std::vector<nccu::gfx::Color> rectColors;
    std::vector<std::string> texts;
    std::vector<float>       textX;
    std::vector<int>         textSize;
    void DrawRect(nccu::gfx::Rect, nccu::gfx::Color c) override {
        ++rects; rectColors.push_back(c);
    }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {}
    void DrawText(std::string_view t, nccu::gfx::Vec2 p, int sz,
                  nccu::gfx::Color) override {
        texts.emplace_back(t);
        textX.push_back(p.x);
        textSize.push_back(sz);
    }
};
bool Has(const Spy& s, std::string_view needle) {
    for (const std::string& t : s.texts)
        if (t.find(needle) != std::string::npos) return true;
    return false;
}
bool HasRectRGB(const Spy& s, nccu::gfx::Color want) {
    for (const auto& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

} // namespace

// ---- Transition classification (the bookend rule) ----------------------

TEST_CASE("ChapterCardForTransition: a chapter START fires the Lost card") {
    // Game start (sentinel -> Ch1) and every Interlude -> chapter exit.
    CHECK(ChapterCardForTransition(S::Ending_C, S::Chapter1_AddDrop) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter2_Midterms) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter3_SportsDay) == ChapterCardKind::Lost);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Chapter4_Finals) == ChapterCardKind::Lost);
}

TEST_CASE("ChapterCardForTransition: a chapter CLEAR fires the Found card") {
    // Ch1/2/3 -> the market (the chapter just cleared after its closing
    // narration closed). Ch4 never reaches here (it goes to an ending).
    CHECK(ChapterCardForTransition(S::Chapter1_AddDrop, S::Interlude_Market) == ChapterCardKind::Found);
    CHECK(ChapterCardForTransition(S::Chapter2_Midterms, S::Interlude_Market) == ChapterCardKind::Found);
    CHECK(ChapterCardForTransition(S::Chapter3_SportsDay, S::Interlude_Market) == ChapterCardKind::Found);
}

TEST_CASE("ChapterCardForTransition: endings fire NO card (EndingView owns them)") {
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_A) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_B) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_D) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Chapter4_Finals, S::Ending_C) == ChapterCardKind::None);
}

TEST_CASE("ChapterCardForTransition: a same-state 'transition' fires nothing") {
    CHECK(ChapterCardForTransition(S::Chapter2_Midterms, S::Chapter2_Midterms) == ChapterCardKind::None);
    CHECK(ChapterCardForTransition(S::Interlude_Market, S::Interlude_Market) == ChapterCardKind::None);
}

// ---- Copy ---------------------------------------------------------------

TEST_CASE("Headline: Ch1 uses the inciting variant, Ch2-4 the recurring one") {
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter1_AddDrop) == "傘，不見了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter2_Midterms) == "傘又掉了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Lost, S::Chapter4_Finals) == "傘又掉了");
    CHECK(ChapterCardHeadline(ChapterCardKind::Found, S::Interlude_Market) == "找到傘了");
    CHECK(ChapterCardHeadline(ChapterCardKind::None, S::Chapter1_AddDrop) == "");
}

TEST_CASE("Subtitle: a Lost card names the starting chapter") {
    CHECK(ChapterCardSubtitle(ChapterCardKind::Lost, S::Chapter2_Midterms) == "第二章 期中考");
    CHECK(ChapterCardSubtitle(ChapterCardKind::Found, S::Interlude_Market) == "這一章，過去了");
}

// ---- The deterministic timer state machine ------------------------------

TEST_CASE("ChapterCardState: fade-in -> hold -> fade-out -> auto-clear") {
    ChapterCardState c;
    CHECK_FALSE(c.Active());
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第二章 期中考");
    CHECK(c.Active());
    CHECK(c.Kind() == ChapterCardKind::Lost);
    CHECK(c.Headline() == "傘又掉了");
    CHECK(c.Subtitle() == "第二章 期中考");

    // At t=0 alpha is ~0 (just armed), ramps up over kFade.
    CHECK(c.Alpha() == doctest::Approx(0.0f));
    c.Step(ChapterCardState::kFade * 0.5f);
    CHECK(c.Alpha() == doctest::Approx(0.5f));        // mid fade-in
    c.Step(ChapterCardState::kFade * 0.5f);
    CHECK(c.Alpha() == doctest::Approx(1.0f));        // reached full hold

    // Hold: still 1.0 partway through.
    c.Step(0.5f);
    CHECK(c.Alpha() == doctest::Approx(1.0f));

    // Drive to within the fade-out window and confirm it dips below 1.
    while (c.Active() && c.Elapsed() < ChapterCardState::kTotal - ChapterCardState::kFade * 0.5f)
        c.Step(0.01f);
    CHECK(c.Active());
    CHECK(c.Alpha() < 1.0f);
    CHECK(c.Alpha() > 0.0f);

    // Past kTotal it auto-clears.
    c.Step(ChapterCardState::kFade);
    CHECK_FALSE(c.Active());
    CHECK(c.Alpha() == doctest::Approx(0.0f));
}

TEST_CASE("ChapterCardState: Dismiss clears immediately (key-press skip)") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Found, "找到傘了", "這一章，過去了");
    c.Step(0.5f);
    CHECK(c.Active());
    c.Dismiss();
    CHECK_FALSE(c.Active());
    CHECK(c.Kind() == ChapterCardKind::None);
}

TEST_CASE("ChapterCardState: reducedMotion is opaque immediately, no ramp") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第三章 運動會");
    CHECK(c.Alpha(/*reducedMotion=*/true) == doctest::Approx(1.0f));  // t=0 opaque
    c.Step(0.5f);
    CHECK(c.Alpha(true) == doctest::Approx(1.0f));
}

TEST_CASE("ChapterCardState: Step on an inactive card is a safe no-op") {
    ChapterCardState c;
    c.Step(1.0f);
    CHECK_FALSE(c.Active());
    CHECK(c.Alpha() == doctest::Approx(0.0f));
}

// ---- DrawChapterCard (spy) ---------------------------------------------

TEST_CASE("DrawChapterCard: inactive card draws nothing") {
    ChapterCardState c;
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(r.rects == 0);
    CHECK(r.texts.empty());
}

TEST_CASE("DrawChapterCard: a Lost card draws headline + subtitle + 破傘 cue") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第二章 期中考");
    c.Step(0.5f);                                   // into full-opacity hold
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(r.rects >= 3);                            // backdrop + panel + rules
    CHECK(Has(r, "傘又掉了"));
    CHECK(Has(r, "第二章 期中考"));
    using nccu::gfx::UmbrellaLook;
    // The Lost card cues the GONE umbrella (the 破傘 ribs), NOT a whole one.
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
}

TEST_CASE("DrawChapterCard: a Found card draws 找到傘了 + the 真傘 (blue) cue") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Found, "找到傘了", "這一章，過去了");
    c.Step(0.5f);
    Spy r;
    nccu::DrawChapterCard(r, c, 800.0f, 450.0f);
    CHECK(Has(r, "找到傘了"));
    using nccu::gfx::UmbrellaLook;
    // The Found card cues the recovered 真傘 (blue), NOT the broken ribs.
    CHECK(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    CHECK_FALSE(HasRectRGB(r, nccu::gfx::UmbrellaLookColor(UmbrellaLook::FragileBroken)));
}

// UI-B-3 — no chapter-card headline/subtitle row spills the panel. The card
// now wraps headline + subtitle (nccu::dialog::WrapToCells) within a side
// margin, so every drawn row's right edge stays on-screen, even at a narrow
// width. Asserted via the shared cell model (~size/2 px per EAW cell).
TEST_CASE("UI-B-3: every chapter-card row stays within the screen width") {
    ChapterCardState c;
    c.Trigger(ChapterCardKind::Lost, "傘又掉了", "第一章 加退選");
    c.Step(0.5f);
    for (const float screenW : {800.0f, 480.0f, 360.0f}) {
        Spy r;
        nccu::DrawChapterCard(r, c, screenW, 450.0f);
        REQUIRE(r.texts.size() == r.textX.size());
        REQUIRE(r.texts.size() == r.textSize.size());
        for (std::size_t i = 0; i < r.texts.size(); ++i) {
            const float perCell = static_cast<float>(r.textSize[i]) * 0.5f;
            const float rowPx =
                static_cast<float>(nccu::dialog::CellWidth(r.texts[i])) * perCell;
            INFO("screenW=" << screenW << " row='" << r.texts[i] << "'");
            CHECK(r.textX[i] >= -0.5f);
            CHECK(r.textX[i] + rowPx <= screenW + 2.0f);
        }
    }
}
