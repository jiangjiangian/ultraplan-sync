#ifndef INTERLUDE_EXIT_H_
#define INTERLUDE_EXIT_H_
#include "gfx/Vec2.h"

namespace nccu {

// The Interlude (四維道常態市集) reuses the single campus z-plane. It has
// no `## NPC：` dialog section in interlude_market.md, so per design
// decision F.1-board=C the exit is the GDD-sanctioned "走出市集南端觸發
// 區" (interlude_market.md:7) rather than a board NPC menu — data-driven,
// no hard-coded dialog, no user-managed content edit.
//
// On entering the Interlude the player is repositioned to kInterludeEntry
// (clear of the exit band so a chapter that ended in the south does not
// instantly bounce the player straight back out). Walking south into
// kInterludeExitZone arms Flag_LeaveInterlude, which the existing
// CheckChapterGates Interlude sibling-if consumes -> Transition to
// SemesterStateMachine::InterludeReturnTo(). Pure geometry so the
// predicate is unit-testable without the GUI / baked terrain mask;
// actual walk-reachability of the band is a manual-verify item.

inline constexpr nccu::gfx::Vec2 kInterludeEntry{500.0f, 1500.0f};

// South band across the walkable 四維道 corridor. The player ctor-spawns
// at y=1860 and ambient students walk y~1880 on this road, so y>=1900 is
// the strip just south of known-walkable ground.
inline constexpr float kInterludeExitMinX = 150.0f;
inline constexpr float kInterludeExitMaxX = 1950.0f;
inline constexpr float kInterludeExitMinY = 1900.0f;
inline constexpr float kInterludeExitMaxY = 2048.0f;  // == world::kSize

[[nodiscard]] inline bool InInterludeExitZone(nccu::gfx::Vec2 centre) noexcept {
    return centre.x >= kInterludeExitMinX && centre.x <= kInterludeExitMaxX &&
           centre.y >= kInterludeExitMinY && centre.y <= kInterludeExitMaxY;
}

} // namespace nccu

#endif // INTERLUDE_EXIT_H_
