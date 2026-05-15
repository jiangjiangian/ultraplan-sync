#include "EndingView.h"
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

// The first 開場字卡 of each ending. Ending C's line is the opening
// 字卡 in docs/content/ending_c.md §2. Endings A/B carry a placeholder
// until Phase 2 接入 their full三結局演出.
std::string_view caption(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_C:
            return "「這樣以後再也不會有人拿錯你的傘了。」";
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
            return "（結局字卡待 Phase 2 接入）";
        default:
            return "";
    }
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
    r.DrawText(std::string(title),
               Vec2{screenW * 0.5f - 60.0f, screenH * 0.40f}, 32,
               Color{255, 255, 255, a});
    r.DrawText(std::string(caption(state)),
               Vec2{screenW * 0.5f - 220.0f, screenH * 0.40f + 48.0f}, 18,
               Color{230, 230, 230, a});
}

} // namespace nccu
