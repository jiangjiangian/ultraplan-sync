#ifndef ENDING_VIEW_H_
#define ENDING_VIEW_H_
#include "state/SemesterState.h"
#include <string_view>

namespace nccu {
namespace gfx { class IRenderer; }

// True for Ending_A / Ending_B / Ending_C.
[[nodiscard]] bool IsEndingState(SemesterState s) noexcept;

// Full-screen ending card: black backdrop + title + a single opening
// 字卡, all at `alpha` (0..1, the fade-in level). Self-contained and
// spy-testable like DrawDialog — View early-returns into this when the
// semester reaches an ending. 1d wires Ending C's real 字卡; A/B carry
// a placeholder pending Phase 2's full三結局演出.
void DrawEndingCard(nccu::gfx::IRenderer& r, SemesterState state,
                    std::string_view title, float alpha,
                    float screenW, float screenH);
} // namespace nccu
#endif // ENDING_VIEW_H_
