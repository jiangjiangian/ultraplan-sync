#ifndef GFX_DECORATIONS_H_
#define GFX_DECORATIONS_H_
#include "gfx/SpriteStrip.h"
#include "gfx/Vec2.h"
#include "quest/Chapter3Quest.h"   // kSportsTrackCx/Cy — 操場 centre
#include "state/SemesterState.h"
#include <array>

// The placed AMBIENT decorations — purely cosmetic, no gameplay effect.
// View-side data (NOT GameObjects, NOT in World::Objects()): the View
// loads each strip texture in its ctor and draws the def from the render
// clock, so the simulation and the harness state.jsonl never see them
// (the harness serialises World::Objects(), which these are absent from —
// the run is byte-identical with or without the art). Each is shown only
// while the semester FSM is in its `chapter`, and draws nothing at all if
// its strip PNG is missing (the empty-resources path) — exactly like
// every other asset in this project.
//
// Both are third-party fan art credited in CREDITS.md; the binaries are
// user-managed and must not be committed (CLAUDE.md §5). Drop the
// converted strips (see tools/gif_to_strip.py) at the stripPath below.

namespace nccu::gfx {

// 羅馬廣場 plaza CENTRE. The Ch2 roster comment in
// include/quest/ChapterSpawns.h pins it: "學霸 (bookworm) sits under the
// 羅馬廣場 statue (plaza centre ~1088,960; placed at the south rim
// 1088,1100)". The bookworm NPC is parked at the south RIM (1088,1100) so
// the player can walk up and talk; the STATUE he sits under is the plaza
// CENTRE — (1088,960) — which is where this chiikawa "statue" is drawn.
inline constexpr Vec2 kRomaPlazaCenter{1088.0f, 960.0f};

// The decoration table. Indexed only by the View; order is irrelevant.
//   chiikawa — Ch2 期中考 (Chapter2_Midterms): a pulsing "statue" at the
//       羅馬廣場 centre, ~80px so it reads as a plaza monument (the 24px
//       player stands much smaller beside it); the ping-pong zoom gives
//       the gentle 放大縮小 breathing the owner asked for.
//   cat — Ch3 校慶 (Chapter3_SportsDay): a SMALL cat (~28px — the owner's
//       「小小的一隻，不要太大隻」) at the 操場 centre (kSportsTrackCx/Cy,
//       the same field centre the lap-progress ring is measured around),
//       breathing via the same ping-pong.
inline constexpr std::array<DecorationDef, 2> kDecorations{{
    DecorationDef{SemesterState::Chapter2_Midterms, kRomaPlazaCenter,
                  "resources/assets/decorations/chiikawa_strip.png",
                  /*frameCount=*/8, /*drawScale=*/80.0f, /*fps=*/6.0},
    DecorationDef{SemesterState::Chapter3_SportsDay,
                  Vec2{kSportsTrackCx, kSportsTrackCy},
                  "resources/assets/decorations/cat_strip.png",
                  /*frameCount=*/8, /*drawScale=*/28.0f, /*fps=*/8.0},
}};

} // namespace nccu::gfx

#endif // GFX_DECORATIONS_H_
