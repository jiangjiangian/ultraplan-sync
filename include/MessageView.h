#ifndef MESSAGE_VIEW_H_
#define MESSAGE_VIEW_H_
#include <string>

namespace nccu { namespace gfx { class IRenderer; } }

namespace nccu {

// How long a ShowMessage banner stays on screen, and how long its
// tail-end fade lasts (the last kHudFade seconds of kHudTtl). Exposed so
// the reactive test can pick ages either side of the TTL without
// hard-coding magic numbers.
inline constexpr float kHudTtl  = 4.0f;
inline constexpr float kHudFade = 1.0f;

// Transient bottom-centre toast for the latest EventType::ShowMessage
// (quest cues / 喚醒提示 / 章節清關旁白 / ripple reactions / vendor text),
// mirrored into World by the EventBus subscriber. Pure function of its
// inputs, spy-testable like DrawDialog / DrawEndingCard — no retained
// state. Draws nothing when `message` is empty or `age` >= kHudTtl;
// otherwise a semi-opaque backdrop plus the (CJK-wrapped) text, the
// whole banner fading to transparent over the final kHudFade seconds.
// reducedMotion (audit D8 / SC 2.3.3, default false → existing
// behaviour) collapses the tail-end fade to a hard cut so flashing-
// light-sensitive players see a steady toast that disappears at TTL.
void DrawHudMessage(nccu::gfx::IRenderer& r, const std::string& message,
                    float age, float screenW, float screenH,
                    bool reducedMotion = false);

} // namespace nccu
#endif // MESSAGE_VIEW_H_
