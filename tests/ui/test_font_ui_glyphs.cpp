#include "doctest/doctest.h"
#include "gfx/Font.h"
#include "world/Buildings.h"
#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

// Guards the V1 fix (DialogView.cpp B4 "▼ more" pagination affordance)
// and the V3 ending-card captions at the font-atlas level.
//
// Why this matters / what reverting breaks:
//   nccu::gfx::detail::CollectCodepoints() builds the glyph set baked
//   into the CJK font atlas: ASCII 32..126, every codepoint in
//   docs/content/*.md, then UiLiteralChars(). DialogView draws U+25BC
//   (▼) as the "press advance for more" cue; it is NOT ASCII and does
//   not occur in any content .md, so it only reaches the atlas via
//   UiLiteralChars(). If the V1 line ("\xE2\x96\xBC" in
//   include/gfx/Font.h::UiLiteralChars) is reverted, 0x25BC drops out
//   of CollectCodepoints(), raylib renders the no-glyph `?` fallback,
//   and B4's pagination affordance becomes a meaningless tofu `?`
//   (exactly the c3_b dialog+tofu regression the gfx pass captured).
//   The first TEST_CASE below then fails.
//
//   The ending captions (V3) likewise live in UiLiteralChars(): 討/厭
//   in Ending B's 字卡 occur in NO content file, so reverting the V3
//   literal additions would tofu the bad-ending card even when
//   docs/content is readable. The second TEST_CASE pins those glyphs.
//
// Pure logic only — CollectCodepoints() reads files + uses raylib's
// codepoint decoder but needs no GL context, so this is headless-safe
// like the other tests/*.cpp doctest units (see test_color.cpp).

using nccu::gfx::detail::CollectCodepoints;

namespace {

bool Contains(const std::vector<int>& v, int cp) {
    return std::find(v.begin(), v.end(), cp) != v.end();
}

} // namespace

TEST_CASE("CollectCodepoints bakes the U+25BC down-cue (V1 ▼ fix)") {
    const std::vector<int> cps = CollectCodepoints();
    // 0x25BC ▼ : DialogView's B4 pagination affordance. Reverting the
    // UiLiteralChars() ▼ entry removes this and the cue tofus to `?`.
    CHECK(Contains(cps, 0x25BC));
    // No ▲ up-cue is used anywhere (DialogView draws only the down-cue),
    // so 0x25B2 is intentionally NOT required by this game's UI.
}

TEST_CASE("CollectCodepoints bakes the V3 ending-caption glyphs") {
    const std::vector<int> cps = CollectCodepoints();
    // 討 (U+8A0E) and 厭 (U+53AD) appear ONLY in Ending B's 字卡
    // 「你成為了你曾經最討厭的那種人」 — they are in no docs/content
    // file, so they reach the atlas exclusively through UiLiteralChars().
    CHECK(Contains(cps, 0x8A0E));   // 討
    CHECK(Contains(cps, 0x53AD));   // 厭
    // CJK quote brackets used by every ending caption.
    CHECK(Contains(cps, 0x300C));   // 「
    CHECK(Contains(cps, 0x300D));   // 」
    // Sanity: ASCII is always present (the always-added 32..126 block).
    CHECK(Contains(cps, static_cast<int>('A')));
}

// REQUIREMENT #10: every glyph used by a building name must be in the
// atlas. View.cpp draws "Inside: " + World::CurrentBuildingName() from
// nccu::buildings::kAll; a name glyph absent from both docs/content and
// UiLiteralChars() renders as the no-glyph `?` (the reported 缺字 bug —
// 井/仁/勇/塘/夫/志/泳/雩 were missing because they occur in no content
// file). This drives the REAL Buildings.h table, so a future rename that
// introduces an uncovered glyph fails here too. Reverting the Font.h
// building-name literal block drops those 8 glyphs and this fails.
TEST_CASE("CollectCodepoints covers every Buildings.h name glyph (#10)") {
    const std::vector<int> cps = CollectCodepoints();
    for (const auto& b : nccu::buildings::kAll) {
        // Decode the UTF-8 building name to codepoints via raylib's own
        // decoder (same path CollectCodepoints uses for the literals).
        const std::string name{b.name};
        int n = 0;
        int* dec = ::LoadCodepoints(name.c_str(), &n);
        for (int i = 0; i < n; ++i) {
            if (dec[i] <= 0) continue;
            INFO("building '" << name << "' glyph U+" << std::hex << dec[i]);
            CHECK(Contains(cps, dec[i]));
        }
        ::UnloadCodepoints(dec);
    }
    // Spot-check the 8 that were the actual reported defect (in no
    // docs/content file → atlas only via the #10 UiLiteralChars block).
    CHECK(Contains(cps, 0x4E95));   // 井  (井塘樓)
    CHECK(Contains(cps, 0x4EC1));   // 仁  (大仁樓)
    CHECK(Contains(cps, 0x52C7));   // 勇  (大勇樓)
    CHECK(Contains(cps, 0x5858));   // 塘  (井塘樓)
    CHECK(Contains(cps, 0x592B));   // 夫  (果夫樓)
    CHECK(Contains(cps, 0x5FD7));   // 志  (志希樓)
    CHECK(Contains(cps, 0x6CF3));   // 泳  (游泳館)
    CHECK(Contains(cps, 0x96E9));   // 雩  (風雩樓/風雩走廊)
}
