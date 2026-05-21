#include "ui/EndingView.h"
#include "dialog/DialogLayout.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"
#include <algorithm>
#include <string>

namespace nccu {

using namespace nccu::gfx;

bool IsEndingState(SemesterState s) noexcept {
    return s == SemesterState::Ending_A ||
           s == SemesterState::Ending_B ||
           s == SemesterState::Ending_C;
}

namespace {

// The opening 字卡 of each ending, grounded in 遊戲企劃與敘事架構.md §陸:
//   A 結局A 完美通關 (True)  — 真相大白：雨過天晴，傘回到你手上。
//   B 結局B 屠龍者終成惡龍   — 詛咒傳承：the achievement card the GDD
//       literally specifies, 「你成為了你曾經最討厭的那種人」.
//   C 結局C 破財消災 (Normal) — unchanged, the opening 字卡 already in
//       docs/content/ending_c.md §2.
// Kept short (≤ the dialog-box feel) and free of any external tool /
// model brand string (forbidden-string rule).
std::string_view caption(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A:
            return "「雨過天晴，傘還在你手上。」";
        case SemesterState::Ending_B:
            return "「你成為了你曾經最討厭的那種人」";
        case SemesterState::Ending_C:
            return "「這樣以後再也不會有人拿錯你的傘了。」";
        default:
            return "";
    }
}

// Horizontal centring without a renderer-side MeasureText (IRenderer
// exposes none — see MessageView.cpp). nccu::dialog::CellWidth is the
// project's font-independent text-measure helper (East-Asian-Width: CJK
// = 2 cells, ASCII = 1), the same cell estimation View.cpp uses for the
// objective panel. At font size `sz` a full-width glyph advances ~`sz`
// px and a narrow one ~`sz/2`, i.e. ~`sz/2` px per cell — so the pixel
// width of `s` is ≈ CellWidth(s) * sz/2. Returns the x that centres it.
float CenteredX(const std::string& s, int sz, float screenW) {
    const float w = static_cast<float>(nccu::dialog::CellWidth(s)) *
                    (static_cast<float>(sz) * 0.5f);
    const float x = screenW * 0.5f - w * 0.5f;
    return x < 0.0f ? 0.0f : x;
}

// Ending B is the 灰暗 (grey-toned) timeline per the GDD ("畫面色調永久
// 轉為灰暗"): desaturate the otherwise-white title/caption toward grey so
// the bad ending reads visibly colder than A/C. Pure presentation.
Color endingTextColor(SemesterState s, unsigned char a) {
    if (s == SemesterState::Ending_B) return Color{150, 150, 155, a};
    return Color{255, 255, 255, a};
}

}  // namespace

void DrawEndingCard(IRenderer& r, SemesterState state,
                    std::string_view title, float alpha,
                    float screenW, float screenH) {
    alpha = std::min(1.0f, std::max(0.0f, alpha));
    const unsigned char a = static_cast<unsigned char>(alpha * 255.0f);

    // Self-contained fade: the backdrop carries the same alpha so the
    // spy test sees a real card even though View also early-returns.
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, a});

    // Title + 字卡 are horizontally centred by measuring each string's
    // width in font cells (CellWidth) instead of the old hardcoded
    // screenW*0.5 - 60 / - 220 offsets, which left long/short CJK
    // captions visibly off-centre. Ending B is greyed per the GDD.
    constexpr int kTitleSize   = 32;
    constexpr int kCaptionSize = 18;
    const std::string ttl{title};
    const std::string cap{caption(state)};
    const Color tint = endingTextColor(state, a);
    r.DrawText(ttl,
               Vec2{CenteredX(ttl, kTitleSize, screenW), screenH * 0.40f},
               kTitleSize, tint);
    r.DrawText(cap,
               Vec2{CenteredX(cap, kCaptionSize, screenW),
                    screenH * 0.40f + 48.0f},
               kCaptionSize, tint);
}

} // namespace nccu
