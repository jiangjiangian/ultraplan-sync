#include "doctest/doctest.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/FragileUmbrella.h"
#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/CursedUmbrella.h"
#include "engine/render/IRenderer.h"

#include <set>
#include <tuple>
#include <vector>

namespace {

// Spy IRenderer: records every primitive instead of touching a GL context,
// so the polymorphic Render() path is testable headless.
struct CountingRenderer final : nccu::gfx::IRenderer {
    struct RectCall { nccu::gfx::Rect r; nccu::gfx::Color c; };
    std::vector<RectCall> rects;
    int sprites = 0;
    int texts = 0;

    void DrawRect(nccu::gfx::Rect r, nccu::gfx::Color c) override {
        rects.push_back({r, c});
    }
    void DrawSprite(const nccu::gfx::Texture&, nccu::gfx::Rect,
                    nccu::gfx::Rect, nccu::gfx::Color) override {
        ++sprites;
    }
    void DrawText(std::string_view, nccu::gfx::Vec2, int,
                  nccu::gfx::Color) override {
        ++texts;
    }
};

// A canonical fingerprint of the whole glyph (every rect's geometry +
// colour, in draw order). Two umbrella styles are "the same" iff this
// matches — the pre-#9 defect (all four identical but for tint).
std::vector<std::tuple<float,float,float,float,int,int,int,int>>
Fingerprint(const CountingRenderer& spy) {
    std::vector<std::tuple<float,float,float,float,int,int,int,int>> fp;
    for (const auto& rc : spy.rects)
        fp.emplace_back(rc.r.x, rc.r.y, rc.r.width, rc.r.height,
                        rc.c.r, rc.c.g, rc.c.b, rc.c.a);
    return fp;
}

template <class U>
CountingRenderer DrawOf() {
    U u{nccu::gfx::Vec2{100.0f, 200.0f}};
    CountingRenderer spy;
    u.Render(spy);
    return spy;
}

} // namespace

// REQUIREMENT #9: the umbrellas must look CLEARLY different. The old
// contract (one shared 3-rect skeleton, only the tint differs) WAS the
// defect — four near-identical pale glyphs. The new contract: each
// UmbrellaStyle draws its own distinct silhouette in its own bold,
// well-separated tint, while staying a pure rect-only vector glyph
// (no sprite/text — MVC: View renders off the object's own data).
//
// Revert-verify: collapsing Render() back to one shared rect set (or
// restoring the near-identical pale tints) makes the "all four
// fingerprints distinct" / "tints well separated" CHECKs fail.
TEST_CASE("REQ#9: the four umbrellas each render a DISTINCT glyph") {
    const CountingRenderer t = DrawOf<TrueUmbrella>();
    const CountingRenderer f = DrawOf<FragileUmbrella>();
    const CountingRenderer p = DrawOf<ProfessorTrapUmbrella>();
    const CountingRenderer c = DrawOf<CursedUmbrella>();

    // Still a pure vector glyph for every style (no sprite, no text).
    for (const CountingRenderer* s : {&t, &f, &p, &c}) {
        CHECK(s->sprites == 0);
        CHECK(s->texts == 0);
        CHECK(s->rects.size() >= 3);          // a real, drawn umbrella
    }

    // The decisive #9 assertion: all FOUR full glyph fingerprints are
    // pairwise different (geometry and/or colour) — not "same shape,
    // different tint". A std::set of the fingerprints must hold 4.
    using FP = std::vector<std::tuple<float,float,float,float,int,int,int,int>>;
    std::set<FP> distinct{Fingerprint(t), Fingerprint(f),
                          Fingerprint(p), Fingerprint(c)};
    CHECK(distinct.size() == 4);

    // And the SHAPES differ too (not merely a recolour of one skeleton):
    // the rect COUNT differs across the styles, so the silhouettes are
    // genuinely different, not just tinted.
    std::set<std::size_t> rectCounts{
        t.rects.size(), f.rects.size(), p.rects.size(), c.rects.size()};
    CHECK(rectCounts.size() >= 3);            // >=3 of 4 silhouettes differ in rect count
}

// The bold per-umbrella tints must be FAR apart (the pre-#9 tints were
// all pale near-blue: |Δ| of ~10–30 per channel — visually the same).
// Assert every pair of canopy tints differs by a large channel sum so
// they cannot be confused on the map.
TEST_CASE("REQ#9: umbrella canopy tints are boldly separated") {
    auto canopy = [](const CountingRenderer& s) {
        // rects[0] is the topmost canopy slab for every style — its
        // colour is that umbrella's signature tint.
        return s.rects.at(0).c;
    };
    const nccu::gfx::Color ct = canopy(DrawOf<TrueUmbrella>());
    const nccu::gfx::Color cf = canopy(DrawOf<FragileUmbrella>());
    const nccu::gfx::Color cp = canopy(DrawOf<ProfessorTrapUmbrella>());
    const nccu::gfx::Color cc = canopy(DrawOf<CursedUmbrella>());

    auto dist = [](nccu::gfx::Color a, nccu::gfx::Color b) {
        auto d = [](int x, int y) { return x > y ? x - y : y - x; };
        return d(a.r, b.r) + d(a.g, b.g) + d(a.b, b.b);
    };
    // Manhattan colour distance ≥ 120 for every pair — the old pale set
    // (e.g. {180,230,255} vs {200,220,235}) summed only ~45, well under.
    const nccu::gfx::Color all[] = {ct, cf, cp, cc};
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j)
            CHECK(dist(all[i], all[j]) >= 120);
}
