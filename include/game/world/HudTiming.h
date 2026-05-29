#ifndef GAME_WORLD_HUD_TIMING_H_
#define GAME_WORLD_HUD_TIMING_H_

namespace nccu {

// Blueprint Phase 4 — game-side HUD-message timing constants. Moved
// out of ui/MessageView.h so World (which ages the HUD message slots
// against kHudTtl in HudExpired() + DismissHud()) can read them
// without pulling the ui render header — closes a game→ui back-edge.
// ui/MessageView.h still defines DrawHudMessage and reads the same
// kHudTtl / kHudFade through this header.
//
// kHudTtl  — how long a ShowMessage banner stays on screen.
// kHudFade — length of the tail-end fade (last kHudFade seconds
//            of kHudTtl). The fade is collapsed to a hard cut under
//            the reduced-motion accessibility profile.
inline constexpr float kHudTtl  = 4.0f;
inline constexpr float kHudFade = 1.0f;

} // namespace nccu

#endif // GAME_WORLD_HUD_TIMING_H_
