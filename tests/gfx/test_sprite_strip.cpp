#include "doctest/doctest.h"
#include "game/gfx/SpriteStrip.h"
#include "game/gfx/Decorations.h"
#include <cmath>
#include <limits>
#include <vector>

using namespace nccu::gfx;
using namespace nccu::engine::math;
using nccu::SemesterState;
using nccu::kSportsTrackCx;
using nccu::kSportsTrackCy;

// Sample FrameAt at every whole tick t = k/fps for `ticks` ticks, so the
// returned sequence is the per-frame index the View would show. fps here
// is arbitrary (1.0) — the helper only depends on floor(t*fps).
static std::vector<int> Sample(int n, int ticks) {
    std::vector<int> seq;
    seq.reserve(static_cast<std::size_t>(ticks));
    for (int k = 0; k < ticks; ++k)
        seq.push_back(FrameAt(static_cast<double>(k), n, 1.0));
    return seq;
}

TEST_CASE("FrameAt: n=4 yields the ping-pong triangle 0,1,2,3,2,1,0,1,...") {
    // The brief's canonical sequence. Period is 2*(n-1) = 6 ticks, so the
    // 7th sample (index 6) returns to 0 and the pattern repeats.
    const std::vector<int> expected =
        {0, 1, 2, 3, 2, 1, 0, 1, 2, 3, 2, 1, 0, 1};
    CHECK(Sample(4, static_cast<int>(expected.size())) == expected);
}

TEST_CASE("FrameAt: apex is n-1 and is hit exactly once per period") {
    // n=4: only tick 3 (and 3 + 6k) returns 3; the neighbours are 2.
    CHECK(FrameAt(2.0, 4, 1.0) == 2);
    CHECK(FrameAt(3.0, 4, 1.0) == 3);   // apex
    CHECK(FrameAt(4.0, 4, 1.0) == 2);
    CHECK(FrameAt(9.0, 4, 1.0) == 3);   // apex again one period later (3+6)
}

TEST_CASE("FrameAt: never leaves [0, n-1] across a long span") {
    for (int n = 2; n <= 8; ++n) {
        for (int k = 0; k < 500; ++k) {
            const int f = FrameAt(static_cast<double>(k), n, 1.0);
            CHECK(f >= 0);
            CHECK(f <= n - 1);
        }
    }
}

TEST_CASE("FrameAt: two-frame strip bounces 0,1,0,1,... (period 2)") {
    const std::vector<int> expected = {0, 1, 0, 1, 0, 1};
    CHECK(Sample(2, 6) == expected);
}

TEST_CASE("FrameAt: degenerate inputs are total and safe → frame 0") {
    CHECK(FrameAt(5.0, 1, 6.0) == 0);    // single-frame strip = static
    CHECK(FrameAt(5.0, 0, 6.0) == 0);    // no frames
    CHECK(FrameAt(5.0, -3, 6.0) == 0);   // nonsense count
    CHECK(FrameAt(5.0, 4, 0.0) == 0);    // stopped clock
    CHECK(FrameAt(5.0, 4, -1.0) == 0);   // negative fps
    CHECK(FrameAt(std::nan(""), 4, 6.0) == 0);        // NaN clock
    CHECK(FrameAt(std::numeric_limits<double>::infinity(), 4, 6.0) == 0);
}

TEST_CASE("FrameAt: negative time never produces a negative index") {
    for (int k = -50; k < 0; ++k) {
        const int f = FrameAt(static_cast<double>(k), 4, 1.0);
        CHECK(f >= 0);
        CHECK(f <= 3);
    }
}

TEST_CASE("FrameAt: fps controls how many ticks per frame advance") {
    // At fps=6 a frame lasts 1/6 s, so t in [0,1/6) is frame 0 and
    // t in [1/6, 2/6) is frame 1.
    CHECK(FrameAt(0.0,        8, 6.0) == 0);
    CHECK(FrameAt(0.10,       8, 6.0) == 0);   // still < 1/6
    CHECK(FrameAt(1.0 / 6.0,  8, 6.0) == 1);   // crossed into frame 1
    CHECK(FrameAt(2.0 / 6.0,  8, 6.0) == 2);
}

TEST_CASE("StripSourceRect: slices a horizontal strip left-to-right") {
    // 8 frames in a 256x40 strip → each frame is 32x40, tiled on x.
    const Rect r0 = StripSourceRect(0, 8, 256, 40);
    CHECK(r0.x == doctest::Approx(0.0f));
    CHECK(r0.y == doctest::Approx(0.0f));
    CHECK(r0.width == doctest::Approx(32.0f));
    CHECK(r0.height == doctest::Approx(40.0f));

    const Rect r3 = StripSourceRect(3, 8, 256, 40);
    CHECK(r3.x == doctest::Approx(96.0f));   // 3 * 32
    CHECK(r3.width == doctest::Approx(32.0f));

    const Rect rLast = StripSourceRect(7, 8, 256, 40);
    CHECK(rLast.x == doctest::Approx(224.0f));            // 7 * 32
    CHECK(rLast.x + rLast.width == doctest::Approx(256.0f)); // ends at texW
}

TEST_CASE("StripSourceRect: frameCount<=0 degenerates to the whole texture") {
    const Rect r = StripSourceRect(0, 0, 100, 50);
    CHECK(r.width == doctest::Approx(100.0f));
    CHECK(r.height == doctest::Approx(50.0f));
}

TEST_CASE("DecorationDestRect: centred on the anchor, longer side == drawScale") {
    DecorationDef d{SemesterState::Chapter3_SportsDay, Vec2{100.0f, 200.0f},
                    "x", /*frameCount=*/8, /*drawScale=*/40.0f, /*fps=*/8.0};
    // 8 frames in a 256x64 texture → one frame is 32x64; the longer side
    // (64, the height) scales to 40 → scale 0.625 → w=20, h=40.
    const Rect r = DecorationDestRect(d, 256, 64);
    CHECK(r.width == doctest::Approx(20.0f));
    CHECK(r.height == doctest::Approx(40.0f));
    // Centred: the rect's midpoint is the anchor.
    CHECK(r.x + r.width * 0.5f == doctest::Approx(100.0f));
    CHECK(r.y + r.height * 0.5f == doctest::Approx(200.0f));
}

TEST_CASE("DecorationDestRect: wider-than-tall frame scales its width to drawScale") {
    DecorationDef d{SemesterState::Chapter2_Midterms, Vec2{0.0f, 0.0f},
                    "x", /*frameCount=*/4, /*drawScale=*/80.0f, /*fps=*/6.0};
    // 4 frames in a 400x50 texture → one frame is 100x50; longer side is
    // 100 (width) → scale 0.8 → w=80, h=40.
    const Rect r = DecorationDestRect(d, 400, 50);
    CHECK(r.width == doctest::Approx(80.0f));
    CHECK(r.height == doctest::Approx(40.0f));
}

TEST_CASE("A-T2 kDecorations: chiikawa near the 學霸, cat WEST of the 綜院") {
    // The table is the single source of truth the View reads; pin the
    // placement + chapter so an accidental edit is caught here.
    //
    // A-T2 placement (owner nudges):
    //   chiikawa — moved DOWN from the plaza geometric centre (1088,960)
    //     to (1088,1040), 60 px above the 學霸's 1088,1100 post, so it
    //     reads as the statue he is slumped under (近學霸).
    //   cat — moved LEFT from the track centre (1694,740, which sat inside
    //     the 綜合院館 footprint x1681.. and was occluded) to x1530 on the
    //     open western 操場, WEST of the 綜院's 1681 left edge, so it is
    //     visible (放左邊一點才看的到). Same track row y740, still 小小的一隻.
    bool sawChiikawa = false, sawCat = false;
    for (const auto& d : kDecorations) {
        if (d.chapter == SemesterState::Chapter2_Midterms) {
            sawChiikawa = true;
            CHECK(d.center.x == doctest::Approx(1088.0f));
            CHECK(d.center.y == doctest::Approx(1040.0f));   // A-T2: 960 -> 1040
            // Sits just ABOVE the bookworm's south-rim post (1088,1100) so
            // they group as one tableau — never below/past him.
            CHECK(d.center.y < 1100.0f);
            CHECK(d.center.y > 960.0f);                      // genuinely moved down
            CHECK(d.frameCount >= 1);
        } else if (d.chapter == SemesterState::Chapter3_SportsDay) {
            sawCat = true;
            CHECK(d.center.x == doctest::Approx(1530.0f));   // A-T2: 1694 -> 1530
            // The whole point: WEST of the 綜合院館 left edge (1681) so the
            // building no longer paints over it; still on the 操場 field
            // (rect x1384..2005) at the track row.
            CHECK(d.center.x < 1681.0f);                     // not under 綜院
            CHECK(d.center.x > 1384.0f);                     // still on the field
            CHECK(d.center.y == doctest::Approx(kSportsTrackCy));  // 740, track row
            CHECK(d.drawScale <= 32.0f);   // owner: 小小的一隻
            CHECK(d.frameCount >= 1);
        }
    }
    CHECK(sawChiikawa);
    CHECK(sawCat);
}
