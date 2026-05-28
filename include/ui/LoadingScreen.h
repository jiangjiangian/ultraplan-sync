#ifndef UI_LOADING_SCREEN_H_
#define UI_LOADING_SCREEN_H_
#include "engine/render/Window.h"

namespace nccu {

// UI-C-2: the brief 載入中… screen shown ONCE at program start, on the
// HUMAN path only. It draws a clean centered label on a cleared background
// while it warms the process texture cache (gfx::PreloadGameTextures), so
// the subsequent World/View construction (after character-select, and on
// every Restart) hits the warm cache with no first-frame disk/GPU stutter.
//
// Screen-flow ORDER (the owner's 載入畫面邏輯和順序): window/GL up →
// EnsureFont → [this loading screen warms textures] → Title → Select →
// Playing. Placed before the title so the one-time asset cost is paid
// behind an explicit "loading" affordance instead of stuttering the first
// gameplay frame.
//
// HARNESS: NOT called when the autoplay harness is active — main.cpp
// guards it with !harness.Active(), exactly as the title/character-select
// are skipped, so the scripted frame timeline and state.jsonl stay
// byte-identical. (The cache WARM itself is harmless either way; only this
// visible screen is human-gated. The harness path lets World/View warm the
// cache lazily on their first construct, off the recorded frame loop.)
//
// Runs its own short draw loop on `win`; returns early if the window is
// closed during loading (the caller then exits cleanly).
void RunLoadingScreen(gfx::Window& win);

} // namespace nccu

#endif // UI_LOADING_SCREEN_H_
